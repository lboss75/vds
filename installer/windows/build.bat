set root_folder=%~d0%~p0
set src_dir=%root_folder%..\..\build\app\vds_web_server\Debug

"%WIX%bin\candle.exe" -dsrc_dir=%src_dir% vds_web_service.wxs -ext WixUIExtension
"%WIX%bin\light.exe" -cultures:ru-ru -loc "%WIX%SDK\wixui\WixUI_ru-ru.wxl" vds_web_service.wixobj -ext WixUIExtension