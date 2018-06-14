using System;
using System.Runtime.InteropServices;

namespace vds_embedded.net
{
    public class vds_api
    {
        private IntPtr vds_;

        public vds_api(){
            this.vds_ = vds_init();
        }

        public void root_folder(string folder){
            vds_set_root_folder(this.vds_, folder);
        }

        public void server_root(string login, string password){
            vds_server_root(this.vds_, login, password);
            var error = Marshal.PtrToStringAnsi(vds_last_error(this.vds_));
            if(!string.IsNullOrWhiteSpace(error)){
                throw new Exception(error);
            }
        }

        const string lib_embedded = "libvds_embedded";

        [DllImport (lib_embedded)]
        private static extern IntPtr vds_init ();

        [DllImport (lib_embedded, CharSet = CharSet.Ansi)]
        private static extern IntPtr vds_last_error (IntPtr vds);

        [DllImport (lib_embedded, CharSet = CharSet.Ansi)]
        private static extern void vds_set_root_folder (IntPtr vds, string root_folder);

        [DllImport (lib_embedded, CharSet = CharSet.Ansi)]
        private static extern void vds_server_root (IntPtr vds, string login, string password);

    }
}
