using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace vds_wf_client
{
    class vds_api : IDisposable
    {
        private IntPtr vds_;

        public vds_api()
        {
            this.vds_ = vds_init();
        }

        public void start()
        {
            check_error(vds_start(this.vds_));

        }

        private void check_error(IntPtr vds_result)
        {
            if (IntPtr.Zero != vds_result)
            {
                var error = StringFromNativeUtf8(vds_result);
                throw new Exception(error);
            }
        }

        public session login_begin(string login, string password)
        {
            var login_ptr = NativeUtf8FromString(login);
            var password_ptr = NativeUtf8FromString(password);
            try
            {
                return new session(vds_login(this.vds_, login_ptr, password_ptr));
            }
            finally
            {
                Marshal.FreeHGlobal(login_ptr);
                Marshal.FreeHGlobal(password_ptr);
            }
        }

        public class session : IDisposable
        {
            private IntPtr session_;

            public session(IntPtr session_ptr)
            {
                this.session_ = session_ptr;
            }

            public void Dispose()
            {
                vds_session_destroy(session_);
            }
        }

        public void root_folder(string folder)
        {
            var folder_ptr = NativeUtf8FromString(folder);
            try
            {
                vds_set_root_folder(this.vds_, folder_ptr);
            }
            finally
            {
                Marshal.FreeHGlobal(folder_ptr);
            }
        }

        public void server_root(string login, string password)
        {
            var login_ptr = NativeUtf8FromString(login);
            var password_ptr = NativeUtf8FromString(password);
            try
            {
                check_error(vds_server_root(this.vds_, login_ptr, password_ptr));
            }
            finally
            {
                Marshal.FreeHGlobal(login_ptr);
                Marshal.FreeHGlobal(password_ptr);
            }
        }
#if __linux__
        const string lib_embedded = "libvds_embedded";
#else
        const string lib_embedded = "vds_embedded";
#endif
        [DllImport(lib_embedded, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr vds_init();

        [DllImport(lib_embedded, CharSet = CharSet.Ansi)]
        private static extern void vds_done(IntPtr vds);

        [DllImport(lib_embedded, CharSet = CharSet.Ansi)]
        private static extern IntPtr vds_start(IntPtr vds);


        [DllImport(lib_embedded, CharSet = CharSet.Ansi)]
        private static extern IntPtr vds_last_error(IntPtr vds);

        [DllImport(lib_embedded, CharSet = CharSet.Ansi)]
        private static extern IntPtr vds_set_root_folder(IntPtr vds, IntPtr root_folder);

        [DllImport(lib_embedded, CharSet = CharSet.Ansi)]
        private static extern IntPtr vds_server_root(IntPtr vds, IntPtr login, IntPtr password);

        [DllImport(lib_embedded, CharSet = CharSet.Ansi)]
        private static extern IntPtr vds_login(IntPtr vds, IntPtr login, IntPtr password);
        [DllImport(lib_embedded, CharSet = CharSet.Ansi)]
        private static extern void vds_session_destroy(IntPtr vds_session);

        public void Dispose()
        {
            vds_done(this.vds_);
        }
        
        private static IntPtr NativeUtf8FromString(string managedString)
        {
            int len = Encoding.UTF8.GetByteCount(managedString);
            byte[] buffer = new byte[len + 1];
            Encoding.UTF8.GetBytes(managedString, 0, managedString.Length, buffer, 0);
            IntPtr nativeUtf8 = Marshal.AllocHGlobal(buffer.Length);
            Marshal.Copy(buffer, 0, nativeUtf8, buffer.Length);
            return nativeUtf8;
        }

        private static string StringFromNativeUtf8(IntPtr nativeUtf8)
        {
            int len = 0;
            while (Marshal.ReadByte(nativeUtf8, len) != 0) ++len;
            byte[] buffer = new byte[len];
            Marshal.Copy(nativeUtf8, buffer, 0, buffer.Length);
            return Encoding.UTF8.GetString(buffer);
        }
    }
}
