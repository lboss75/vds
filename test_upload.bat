echo Create root
set root_folder=%~d0%~p0

C:\Python37\python.exe C:\Users\v.malyshev\source\repos\s3cmd-2.0.1\s3cmd sync --access_key=vadim@iv-soft.ru --secret_key=123 --no-ssl --host=localhost:8051 --host-bucket=localhost:8051 -v -r www s3://test/www_bucket/testpath