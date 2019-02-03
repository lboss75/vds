/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "app.h"

vds::app * vds::app::the_app_ = nullptr;

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
    f.close();
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

#endif
