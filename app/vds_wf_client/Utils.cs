using System.Windows.Forms;

namespace vds_wf_client
{
    internal class Utils
    {
        public static string FormatSize(long size)
        {
            string[] sizes = { "Б", "К", "М", "Г", "Т" };
            int order = 0;
            while (size >= 1024 && order < sizes.Length - 1)
            {
                order++;
                size = size / 1024;
            }

            return string.Format("{0:0.##} {1}", size, sizes[order]);
        }
    }
}