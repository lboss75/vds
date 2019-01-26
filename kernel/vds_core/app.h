#ifndef __VDS_CORE_APP_H_
#define __VDS_CORE_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <string>
#include <exception>
#include <iostream>

#ifndef _WIN32
#include <sys/resource.h> 
#include <signal.h>
#include <sys/stat.h>
#include <wait.h>
#include "barrier.h"
/* an idea from 'Advanced Programming in the Unix Environment'
  Stevens 1993 - see BecomeDaemonProcess() */

#define OPEN_MAX_GUESS 256
#define CHILD_NEED_TERMINATE 9

#endif

#include "service_provider.h"
#include "logger.h"
#include "command_line.h"
#include "filename.h"
#include "persistence.h"
#include "file.h"
#include "vds_exceptions.h"

namespace vds{
  
  class app
  {
  public:
    app() {
      the_app_ = this;
    }

    std::string app_name() const {
      return "VDS application";
    }
    
    std::string app_description() const {
      return "Distributed file system";
    }
    
    std::string app_version() const {
      return "0.1";
    }
    
  protected:
    static app * the_app_;

#ifndef _WIN32
    static barrier stop_barrier;
    static void kill_prev(const foldername & root_folder, const std::string & process_name);
    static void demonize(const foldername & root_folder, const std::string & process_name);
#endif

  };
  
  template <typename app_impl>
  class app_base : public app {
  public:
      app_base()
              : logger_(log_level::ll_info, std::unordered_set<std::string>()),
                log_level_("ll", "log_level", "Log Level", "Set log level"),
                log_modules_("lm", "log_modules", "Log modules", "Set log modules"),
                root_folder_("", "root-folder", "Root folder", "Root folder to store files"),
                current_command_set_(nullptr),
                help_cmd_set_("Show help", "Show application help", "help"),
                help_cmd_switch_("h", "help", "Help", "Show help") {
      }

      int run(int argc, const char **argv) {
        auto result = run_app(argc, argv);
        if(result.has_error()) {
          std::cerr << result.error()->what() << "\n";
          return 1;
        }
        return result.value();
      }

      expected<int> run_app(int argc, const char **argv) {
        this->current_process_ = filename(argv[0]);
#ifndef _WIN32
        // core dumps may be disallowed by parent of this process; change that
        struct rlimit core_limits;
        core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
        setrlimit(RLIMIT_CORE, &core_limits);
#endif
        setlocale(LC_ALL, "Russian");

        auto pthis = static_cast<app_impl *>(this);
        command_line cmd_line(
          pthis->app_name(),
          pthis->app_description(),
          pthis->app_version()
        );

        pthis->register_command_line(cmd_line);
        pthis->register_common_parameters(cmd_line);

        GET_EXPECTED_VALUE(this->current_command_set_, cmd_line.parse(argc, argv));

        pthis->process_common_parameters();

        if (
          nullptr == this->current_command_set_
          || this->current_command_set_ == &this->help_cmd_set_) {
          cmd_line.show_help(this->current_process_.name_without_extension());
          return 1;
        }

        if (pthis->need_demonize()) {
          return pthis->demonize();
        }
        else {
          CHECK_EXPECTED(pthis->start());
          return 2;
        }
      }

  protected:
      file_logger logger_;
      command_line_value log_level_;
      command_line_value log_modules_;
      command_line_value root_folder_;
      const command_line_set *current_command_set_;
      filename current_process_;

      expected<void> start() {
          vds::service_registrator registrator;

          auto pthis = static_cast<app_impl *>(this);
          pthis->register_services(registrator);

          if (!this->root_folder_.value().empty()) {
              vds::foldername folder(this->root_folder_.value());
            CHECK_EXPECTED(folder.create());

              registrator.current_user(folder);
              registrator.local_machine(folder);
          }

          GET_EXPECTED(sp, registrator.build());
          CHECK_EXPECTED(pthis->prepare(sp));
          CHECK_EXPECTED(pthis->start_services(registrator, sp));
          CHECK_EXPECTED(pthis->before_main(sp));
          CHECK_EXPECTED(pthis->main(sp));

          return registrator.shutdown();
      }

      void register_services(service_registrator &registrator) {
          registrator.add(this->logger_);
      }

      expected<void> start_services(service_registrator &registrator, service_provider * /*sp*/) {
        CHECK_EXPECTED(registrator.start());
          this->logger_.debug("core", "Start application");
          return expected<void>();
      }

      expected<void> before_main(service_provider * /*sp*/) {
        return expected<void>();
      }

      expected<void> prepare(service_provider * /*sp*/) {
        return expected<void>();
      }

      void register_command_line(command_line &cmd_line) {
          cmd_line.add_command_set(this->help_cmd_set_);
          this->help_cmd_set_.required(this->help_cmd_switch_);
      }

      void register_common_parameters(command_line &cmd_line) {
          cmd_line.register_common_parameter(this->log_level_);
          cmd_line.register_common_parameter(this->log_modules_);
          cmd_line.register_common_parameter(this->root_folder_);
      }

      void process_common_parameters() {
          if ("trace" == this->log_level_.value()) {
              this->logger_.set_log_level(log_level::ll_trace);
          } else if ("debug" == this->log_level_.value()) {
              this->logger_.set_log_level(log_level::ll_debug);
          } else if ("info" == this->log_level_.value()) {
              this->logger_.set_log_level(log_level::ll_info);
          } else if ("error" == this->log_level_.value()) {
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
              } else {
                  this->logger_.set_log_module(std::string(p, s - p));
                  p = s + 1;
              }
          }
      }

      bool need_demonize() {
          return false;
      }

#ifdef _WIN32
      TCHAR * service_name() const {
        throw vds_exceptions::invalid_operation();
      }

      expected<int> demonize()
      {
        SERVICE_TABLE_ENTRY DispatchTable[] =
        {
          { static_cast<app_impl *>(this)->service_name(), (LPSERVICE_MAIN_FUNCTION)SvcMain },
          { NULL, NULL }
        };

        // This call returns when the service has stopped.
        // The process should simply terminate when the call returns.

        if (!StartServiceCtrlDispatcher(DispatchTable)) {
          DWORD error = GetLastError();
          return vds::make_unexpected<std::system_error>(error, std::system_category(), "StartServiceCtrlDispatcher");
        }

        return 0;
      }

      static VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
      {
        static_cast<app_impl *>(the_app_)->service_main();
      }

      SERVICE_STATUS          gSvcStatus;
      SERVICE_STATUS_HANDLE   gSvcStatusHandle;
      HANDLE                  ghSvcStopEvent;

      void service_main()
      {
        gSvcStatusHandle = RegisterServiceCtrlHandler(
          static_cast<app_impl *>(this)->service_name(),
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

      void SvcInit()
      {
        ghSvcStopEvent = CreateEvent(
          NULL,    // default security attributes
          TRUE,    // manual reset event
          FALSE,   // not signaled
          NULL);   // no name

        if (ghSvcStopEvent == NULL) {
          ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
          return;
        }

        //::Sleep(60 * 1000);
        (void)static_cast<app_impl *>(this)->start();
      }

      void waiting_stop_signal() {
        ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
        WaitForSingleObject(ghSvcStopEvent, INFINITE);
        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
      }

      VOID ReportSvcStatus(DWORD dwCurrentState,
        DWORD dwWin32ExitCode,
        DWORD dwWaitHint)
      {
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

      static VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
      {


        switch (dwCtrl)
        {
        case SERVICE_CONTROL_STOP:
          static_cast<app_base *>(the_app_)->ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

          SetEvent(static_cast<app_base *>(the_app_)->ghSvcStopEvent);
          static_cast<app_base *>(the_app_)->ReportSvcStatus(static_cast<app_base *>(the_app_)->gSvcStatus.dwCurrentState, NO_ERROR, 0);

          return;

        case SERVICE_CONTROL_INTERROGATE:
          break;

        default:
          break;
        }
      }
#else//_WIN32

      int demonize() {
          app::kill_prev(
                  foldername(this->root_folder_.value()),
                  this->current_process_.name_without_extension());

          auto cur_pid = fork();
          switch (cur_pid) {
              case 0: /* we are the child process */
                  break;

              case -1: /* error - bail out (fork failing is very bad) */
                  return vds::make_unexpected<std::system_error>(errno, std::system_category(), "initial fork");

              default: /* we are the parent, so exit */
                  return 0;
          }

          app::demonize(
                  foldername(this->root_folder_.value()),
                  this->current_process_.name_without_extension());

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
                      auto pthis = static_cast<app_impl *>(this);
                      pthis->start();
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


      static void signalHandler(int /*signum*/) {
          stop_barrier.set();
      }

      void waiting_stop_signal() {
          signal(SIGINT, signalHandler);
          stop_barrier.wait();
      }

#endif // _WIN32

  private:
      command_line_set help_cmd_set_;
      command_line_switch help_cmd_switch_;
  };
  
  template <typename app_impl>
  class console_app : public app_base<app_impl>
  {
  public:
    console_app()
    {
    }
  };
}

#endif // __VDS_CORE_APP_H_
