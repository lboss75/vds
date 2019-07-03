/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "app.h"


#ifndef _WIN32
vds::barrier vds::app::stop_barrier;

vds::expected<void> vds::app::kill_prev(const vds::foldername &root_folder, const std::string & process_name) {
    chdir("/");

    CHECK_EXPECTED(root_folder.create());

    filename pid_file_name(root_folder, process_name + ".pid");


    file f;
    auto result = f.open(pid_file_name, file::file_mode::open_read);
    if (result.has_error()) {
        auto ex = dynamic_cast<std::system_error *>(result.error().get());
        if (nullptr == ex
            || ex->code().category() != std::system_category()
            || ex->code().value() != ENOENT) {
            return unexpected(std::move(result.error()));
        }
    }
else {

    char buffer[20];
    GET_EXPECTED(readed, f.read(buffer, sizeof(buffer) - 1));
    buffer[readed] = 0;
    auto pid = strtoul(buffer, (char **) nullptr, 10);

    if (0 == kill(pid, 0)) {
        return vds::make_unexpected<std::runtime_error>(
                string_format(
                        "A lock file %s has been detected. It appears it is owned\nby the (active) process with PID %ld.",
                        pid_file_name.name().c_str(),
                        pid));
    } else {
        auto error = errno;
        if (ESRCH != error) {
            return vds::make_unexpected<std::system_error>(error, std::system_category(),
                                                           "acquire exclusive lock on lock file");
        }
    }
}
    signal(SIGHUP, SIG_IGN);
    return expected<void>();
}

vds::expected<void> vds::app::demonize(const vds::foldername &root_folder, const std::string & process_name) {
    /* make the process a session and process group leader. This simplifies
      job control if we are spawning child servers, and starts work on
      detaching us from a controlling TTY */

    if (setsid() < 0) {
        return vds::make_unexpected<std::system_error>(errno, std::system_category(), "setsid");
    }

    /* ignore SIGHUP as this signal is sent when session leader terminates */
    signal(SIGHUP, SIG_IGN);

    auto pid_str = std::to_string(getpid());
    file f;
    CHECK_EXPECTED(f.open(filename(root_folder, process_name + ".pid"), file::file_mode::truncate));
    CHECK_EXPECTED(f.write(pid_str.c_str(), pid_str.length()));
    CHECK_EXPECTED(f.close());
    /*close open file descriptors */

    auto num_files = sysconf(_SC_OPEN_MAX); /* how many file descriptors? */

    if (num_files < 0) {
        num_files = OPEN_MAX_GUESS; /* from Stevens '93 */
    }

    for (auto i = num_files - 1; 0 <= i; --i) { /* close all open files except lock */
        close(i);
    }

    /* stdin/out/err to /dev/null */
    umask(0); /* set this to whatever is appropriate for you */
    auto stdio_fd = open("/dev/null", O_RDWR); /* fd 0 = stdin */
    dup(stdio_fd); /* fd 1 = stdout */
    dup(stdio_fd); /* fd 2 = stderr */

    /* put server into its own process group. If this process now spawns
      child processes, a signal sent to the parent will be propagated
      to the children */

    setpgrp();

    return expected<void>();
}

        vds::expected<int> vds::app::demonize(const foldername & root_folder) {
          CHECK_EXPECTED(app::kill_prev(
                  root_folder,
                  this->current_process_.name_without_extension()));

          auto cur_pid = fork();
          switch (cur_pid) {
              case 0: /* we are the child process */
                  break;

              case -1: /* error - bail out (fork failing is very bad) */
                  return vds::make_unexpected<std::system_error>(errno, std::system_category(), "initial fork");

              default: /* we are the parent, so exit */
                  return 0;
          }

          CHECK_EXPECTED(app::demonize(
                  root_folder,
                  this->current_process_.name_without_extension()));

          sigset_t sigset;
          sigaddset(&sigset, SIGQUIT);
          sigaddset(&sigset, SIGINT);
          sigaddset(&sigset, SIGTERM);
          sigaddset(&sigset, SIGCHLD);
          sigprocmask(SIG_BLOCK, &sigset, NULL);

          for (bool bContinue = true; bContinue;) {
              auto cur_pid = fork();

              switch (cur_pid) {
                  case 0: {/* we are the child process */
                      vds::service_registrator registrator;
                      CHECK_EXPECTED(this->start(registrator));
                      (void)registrator.shutdown();
                      return 0;
                  }


                  case -1: /* error - bail out (fork failing is very bad) */
                      return vds::make_unexpected<std::system_error>(errno, std::system_category(), "initial fork");

                  default: {/* we are the parent */
                      siginfo_t siginfo;
                      sigwaitinfo(&sigset, &siginfo);
                      if (siginfo.si_signo == SIGCHLD) {
                          int status;
                          wait(&status);
                          status = WEXITSTATUS(status);

                          if (status == CHILD_NEED_TERMINATE) {
                              return CHILD_NEED_TERMINATE;
                          }
                      } else {
                          kill(cur_pid, SIGTERM);
                          bContinue = false;
                      }
                      break;
                  }
              }
          }

          return 0;
      }


      void vds::app::signalHandler(int /*signum*/) {
          stop_barrier.set();
      }

      void vds::app::waiting_stop_signal() {
          signal(SIGINT, signalHandler);
          stop_barrier.wait();
      }
#else

vds::app * vds::app::the_app_ = nullptr;

TCHAR* vds::app::service_name() const {
#ifdef __cpp_exceptions
  throw vds_exceptions::invalid_operation();
#else
  return "Invalid";
#endif
}

vds::expected<int> vds::app::demonize(const foldername & /*root_folder*/) {
  SERVICE_TABLE_ENTRY DispatchTable[] =
  {
    {this->service_name(), (LPSERVICE_MAIN_FUNCTION)SvcMain},
    {NULL, NULL}
  };

  // This call returns when the service has stopped.
  // The process should simply terminate when the call returns.

  if (!StartServiceCtrlDispatcher(DispatchTable)) {
    DWORD error = GetLastError();
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "StartServiceCtrlDispatcher");
  }

  return 0;
}

void vds::app::SvcMain(DWORD /*dwArgc*/, LPTSTR* /*lpszArgv*/) {
  the_app_->service_main();
}

void vds::app::service_main() {
  gSvcStatusHandle = RegisterServiceCtrlHandler(
    this->service_name(),
    SvcCtrlHandler);

  if (!gSvcStatusHandle) {
    return;
  }

  // These SERVICE_STATUS members remain as set here

  gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  gSvcStatus.dwServiceSpecificExitCode = 0;

  ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

  SvcInit();
}

void vds::app::SvcInit() {
  ghSvcStopEvent = CreateEvent(
    NULL, // default security attributes
    TRUE, // manual reset event
    FALSE, // not signaled
    NULL); // no name

  if (ghSvcStopEvent == NULL) {
    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
    return;
  }

  //::Sleep(60 * 1000);
  vds::service_registrator registrator;
  (void)this->start(registrator);
  (void)registrator.shutdown();
}

void vds::app::waiting_stop_signal() {
  ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
  WaitForSingleObject(ghSvcStopEvent, INFINITE);
  ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

void vds::app::ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint) {
  static DWORD dwCheckPoint = 1;

  // Fill in the SERVICE_STATUS structure.

  gSvcStatus.dwCurrentState = dwCurrentState;
  gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
  gSvcStatus.dwWaitHint = dwWaitHint;

  if (dwCurrentState == SERVICE_START_PENDING)
    gSvcStatus.dwControlsAccepted = 0;
  else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

  if ((dwCurrentState == SERVICE_RUNNING) ||
    (dwCurrentState == SERVICE_STOPPED))
    gSvcStatus.dwCheckPoint = 0;
  else gSvcStatus.dwCheckPoint = dwCheckPoint++;

  // Report the status of the service to the SCM.
  SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

void vds::app::SvcCtrlHandler(DWORD dwCtrl) {


  switch (dwCtrl) {
  case SERVICE_CONTROL_STOP:
    the_app_->ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

    SetEvent(the_app_->ghSvcStopEvent);
    the_app_->ReportSvcStatus(the_app_->gSvcStatus.dwCurrentState, NO_ERROR, 0);

    return;

  case SERVICE_CONTROL_INTERROGATE:
    break;

  default:
    break;
  }
}

#endif

const vds::version vds::app::product_version() {
  return version(0, 0, 1);
}

const char* vds::app::brunch() {
  return "test";
}

std::string vds::app::app_name() const {
  return "VDS application";
}

std::string vds::app::app_description() const {
  return "Distributed file system";
}

std::string vds::app::app_version() const {
  return product_version().to_string();
}

vds::expected<void> vds::app::start_services(service_registrator& registrator, service_provider*) {
  CHECK_EXPECTED(registrator.start());
  this->logger_.debug("core", "Start application");
  return expected<void>();
}

vds::expected<void> vds::app::before_main(service_provider*) {
  return expected<void>();
}

vds::expected<void> vds::app::prepare(service_provider*) {
  return expected<void>();
}

void vds::app::register_command_line(command_line& cmd_line) {
  cmd_line.add_command_set(this->help_cmd_set_);
  this->help_cmd_set_.required(this->help_cmd_switch_);
}

void vds::app::register_common_parameters(command_line& cmd_line) {
  cmd_line.register_common_parameter(this->log_level_);
  cmd_line.register_common_parameter(this->log_modules_);
  cmd_line.register_common_parameter(this->root_folder_);
}

void vds::app::process_common_parameters() {
  if ("trace" == this->log_level_.value()) {
    this->logger_.set_log_level(log_level::ll_trace);
  }
  else if ("debug" == this->log_level_.value()) {
    this->logger_.set_log_level(log_level::ll_debug);
  }
  else if ("info" == this->log_level_.value()) {
    this->logger_.set_log_level(log_level::ll_info);
  }
  else if ("error" == this->log_level_.value()) {
    this->logger_.set_log_level(log_level::ll_error);
  }

  auto p = this->log_modules_.value().c_str();
  for (;;) {
    auto s = strchr(p, ',');
    if (nullptr == s) {
      if (0 != *p) {
        this->logger_.set_log_module(p);
      }
      break;
    }
    else {
      this->logger_.set_log_module(std::string(p, s - p));
      p = s + 1;
    }
  }
}

bool vds::app::need_demonize() {
  return false;
}


vds::app::app(): logger_(log_level::ll_info, std::unordered_set<std::string>()),
                 log_level_("ll", "log_level", "Log Level", "Set log level"),
                 log_modules_("lm", "log_modules", "Log modules", "Set log modules"),
                 root_folder_("", "root-folder", "Root folder", "Root folder to store files"),
                 current_command_set_(nullptr),
                 help_cmd_set_("Show help", "Show application help", "help"),
                 help_cmd_switch_("h", "help", "Help", "Show help") {
}

int vds::app::run(int argc, const char** argv) {
  auto result = run_app(argc, argv);
  if (result.has_error()) {
    std::cerr << result.error()->what() << "\n";
    return 1;
  }
  return result.value();
}

vds::expected<int> vds::app::run_app(int argc, const char** argv) {
  this->current_process_ = filename(argv[0]);
#ifndef _WIN32
        // core dumps may be disallowed by parent of this process; change that
        struct rlimit core_limits;
        core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
        setrlimit(RLIMIT_CORE, &core_limits);
#endif
  setlocale(LC_ALL, "Russian");
  auto version = this->app_version();
  if ((brunch() == nullptr || *brunch() == '\0')) {
    version += "-";
    version += brunch();
  }

  command_line cmd_line(
    this->app_name(),
    this->app_description(),
    version
  );

  this->register_command_line(cmd_line);
  this->register_common_parameters(cmd_line);

  GET_EXPECTED_VALUE(this->current_command_set_, cmd_line.parse(argc, argv));

  this->process_common_parameters();

  if (
    nullptr == this->current_command_set_
    || this->current_command_set_ == &this->help_cmd_set_) {
    cmd_line.show_help(this->current_process_.name_without_extension());
    return 1;
  }

  foldername root_folder;
  if(this->root_folder_.value().empty()) {
    GET_EXPECTED_VALUE(root_folder, persistence::current_user(nullptr));
  }
  else {
    root_folder = foldername(this->root_folder_.value());
  }

  CHECK_EXPECTED(root_folder.create());

  foldername tmp_folder(root_folder, "tmp");
  if (tmp_folder.exist()) {
    CHECK_EXPECTED(tmp_folder.files([](const filename & fn)->bool {
      (void)file::delete_file(fn);
      return true;
    }));
  }


  if (this->need_demonize()) {
    return this->demonize(root_folder);
  }
  else {
    vds::service_registrator registrator;
    auto result = this->start(registrator);
    (void)registrator.shutdown();
    CHECK_EXPECTED_ERROR(result);
    return 0;
  }
}

vds::expected<void> vds::app::start(vds::service_registrator& registrator) {
  this->register_services(registrator);

  if (!this->root_folder_.value().empty()) {
    vds::foldername folder(this->root_folder_.value());
    registrator.current_user(folder);
    registrator.local_machine(folder);
  }

  GET_EXPECTED(sp, registrator.build());
  CHECK_EXPECTED(this->prepare(sp));
  CHECK_EXPECTED(this->start_services(registrator, sp));
  CHECK_EXPECTED(this->before_main(sp));
  CHECK_EXPECTED(this->main(sp));

  return expected<void>();
}

void vds::app::register_services(service_registrator& registrator) {
  registrator.add(this->logger_);
}
