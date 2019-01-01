/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "app.h"

vds::app * vds::app::the_app_ = nullptr;

void vds::app::kill_prev(const vds::foldername &root_folder, const std::string & process_name) {
    chdir("/");

    root_folder.create();

    filename pid_file_name(root_folder, process_name + ".pid");

    try {
        file f(pid_file_name, file::file_mode::open_read);

        char buffer[20];
        auto readed = f.read(buffer, sizeof(buffer) - 1);
        buffer[readed] = 0;
        auto pid = strtoul(buffer, (char **) nullptr, 10);

        if (0 == kill(pid, 0)) {
            throw std::runtime_error(
                    string_format(
                            "A lock file %s has been detected. It appears it is owned\nby the (active) process with PID %ld.",
                            pid_file_name.name().c_str(),
                            pid));
        } else {
            auto error = errno;
            if (ESRCH != error) {
                throw std::system_error(error, std::system_category(), "acquire exclusive lock on lock file");
            }
        }
    }
    catch (const std::system_error &ex) {
        if (ex.code().category() != std::system_category()
            || ex.code().value() != ENOENT) {
            std::cout << "Fatal " << ex.code().value() << " " << ex.code().category().name() << "\n";
            throw;
        }
    }

    signal(SIGHUP, SIG_IGN);
}

void vds::app::demonize(const vds::foldername &root_folder, const std::string & process_name) {
    /* make the process a session and process group leader. This simplifies
      job control if we are spawning child servers, and starts work on
      detaching us from a controlling TTY */

    if (setsid() < 0) {
        throw std::system_error(errno, std::system_category(), "setsid");
    }

    /* ignore SIGHUP as this signal is sent when session leader terminates */
    signal(SIGHUP, SIG_IGN);

    auto pid_str = std::to_string(getpid());
    file(filename(root_folder, process_name + ".pid"), file::file_mode::truncate).write(pid_str.c_str(), pid_str.length());
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
}

#ifndef _WIN32
vds::barrier vds::app::stop_barrier;
#endif
