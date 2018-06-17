using System;
using System.Runtime.InteropServices;

namespace vds_app
{
    public class vds_api
    {
        private IntPtr vds_;

        public vds_api()
        {
            this.vds_ = vds_init();
        }

        public void root_folder(string folder)
        {
            vds_set_root_folder(this.vds_, folder);
        }

        public vds_task login(string login, string password)
        {
            var task = vds_login(this.vds_, login, password);
            return new vds_task(task);
        }

        public class vds_task : IDisposable
        {
            private IntPtr task;

            public vds_task(IntPtr task)
            {
                this.task = task;
            }

            public void update()
            {
                var status = new TaskStatus();
                vds_task_status(this.task, status);
                this.state = Marshal.PtrToStringAnsi(status.state);
                this.percent = status.percent;
                this.status_text = Marshal.PtrToStringAnsi(status.status_text);
            }

            public string status_text { get; set; }

            public int percent { get; set; }

            public string state { get; set; }

            public void Dispose()
            {
                vds_task_free(this.task);
            }
        }


        public void server_root(string login, string password){
            vds_server_root(this.vds_, login, password);
            var error = Marshal.PtrToStringAnsi(vds_last_error(this.vds_));
            if(!string.IsNullOrWhiteSpace(error)){
                throw new Exception(error);
            }
        }

        const string lib_embedded = "vds_embedded";

        [DllImport (lib_embedded)]
        private static extern IntPtr vds_init ();

        [DllImport(lib_embedded, CharSet = CharSet.Ansi)]
        private static extern IntPtr vds_login(IntPtr vds, string login, string password);

        [StructLayout(LayoutKind.Sequential)]
        public class TaskStatus
        {
            public IntPtr state;
            public int percent;
            public IntPtr status_text;
        }

        [DllImport(lib_embedded, CharSet = CharSet.Ansi)]
        private static extern IntPtr vds_task_status(IntPtr taskPtr, [Out] TaskStatus status);

        [DllImport(lib_embedded, CharSet = CharSet.Ansi)]
        private static extern void vds_task_free(IntPtr taskPtr);


        [DllImport (lib_embedded, CharSet = CharSet.Ansi)]
        private static extern IntPtr vds_last_error (IntPtr vds);

        [DllImport (lib_embedded, CharSet = CharSet.Ansi)]
        private static extern void vds_set_root_folder (IntPtr vds, string root_folder);

        [DllImport (lib_embedded, CharSet = CharSet.Ansi)]
        private static extern void vds_server_root (IntPtr vds, string login, string password);

    }
}
