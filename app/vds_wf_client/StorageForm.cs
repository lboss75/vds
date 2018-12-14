using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace vds_wf_client
{
    public partial class StorageForm : Form
    {
        public StorageForm()
        {
            InitializeComponent();
        }

        internal vds_api Api { get; set; }
        internal vds_api.session Session { get; set; }

        private void browseBtn_Click(object sender, EventArgs e)
        {
            var dlg = new FolderBrowserDialog();
            dlg.SelectedPath = storagePathEdit.Text;
            dlg.ShowNewFolderButton = true;

            if (dlg.ShowDialog(this) == DialogResult.OK)
            {
                storagePathEdit.Text = dlg.SelectedPath;
            }
        }

        private void okBtn_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrWhiteSpace(nameEdit.Text))
            {
                MessageBox.Show(this, "Укажите название хранилища.", this.Text, MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }

            if (Directory.Exists(storagePathEdit.Text))
            {
                MessageBox.Show(this, "Укажите папку, которой не существует.", this.Text, MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }

            int size = int.Parse(reserveSize.Text);
            if (size <= 0)
            {
                MessageBox.Show(this, "Укажите зарезервированное место.", this.Text, MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }

            try
            {
                Directory.CreateDirectory(storagePathEdit.Text);
                File.WriteAllText(
                    Path.Combine(storagePathEdit.Text, "vds_storage.json"),
                    this.Session.prepare_device_storage());

                this.Session.add_device_storage(nameEdit.Text, storagePathEdit.Text, size);
                DialogResult = DialogResult.OK;
            }
            catch (Exception ex)
            {
                try
                {
                    Directory.Delete(storagePathEdit.Text);
                }
                catch (Exception)
                {

                }

                MessageBox.Show(this, ex.Message, this.Text, MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }
        }

        private void StorageForm_Load(object sender, EventArgs e)
        {
            storagePathEdit.Text = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Personal), ".vds_storage");
        }
    }
}
