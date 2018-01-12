call test_init_root_server.bat

start test_run_server.bat 0 8050

call test_init_server.bat 1 8051
start test_run_server.bat 1 8051