using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace vds.embedded.net
{
    public class VdsApi
    {
        private IntPtr vds_;

        public VdsApi()
        {
            this.vds_ = vds_init();
        }

        public void root_folder(string folder)
        {
            vds_set_root_folder(this.vds_, folder);
        }

        public void server_root(string login, string password)
        {
            vds_server_root(this.vds_, login, password);
            var error = Marshal.PtrToStringAnsi(vds_last_error(this.vds_));
            if (!string.IsNullOrWhiteSpace(error))
            {
                throw new Exception(error);
            }
        }
#if __linux__
        const string LibEmbedded = "libvds_embedded";
#else
        const string LibEmbedded = "vds_embedded";
#endif
        [DllImport(LibEmbedded, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr vds_init();

        [DllImport(LibEmbedded, CharSet = CharSet.Ansi)]
        private static extern IntPtr vds_last_error(IntPtr vds);

        [DllImport(LibEmbedded, CharSet = CharSet.Ansi)]
        private static extern void vds_set_root_folder(IntPtr vds, string root_folder);

        [DllImport(LibEmbedded, CharSet = CharSet.Ansi)]
        private static extern void vds_server_root(IntPtr vds, string login, string password);

    }
}
