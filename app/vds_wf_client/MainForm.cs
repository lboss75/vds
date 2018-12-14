using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace vds_wf_client
{
    public partial class MainForm : Form
    {
        private vds_api api_;
        private vds_api.session session_;
        private bool is_authorized_ = false;

        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Resize(object sender, EventArgs e)
        {
            if (this.WindowState == FormWindowState.Minimized)
            {
                this.ShowInTaskbar = false;
            }
        }

        private void trayNotifyIcon_MouseClick(object sender, MouseEventArgs e)
        {
            if ((e.Button & MouseButtons.Left) != 0)
            {
                SwitchMinimizedState();
            }
        }

        private void SwitchMinimizedState()
        {
            if (this.WindowState == FormWindowState.Minimized)
            {
                this.WindowState = FormWindowState.Normal;
                this.ShowInTaskbar = true;

                if (this.session_ == null)
                {
                    showLogin();
                }
            }
            else
            {
                this.WindowState = FormWindowState.Minimized;
                this.ShowInTaskbar = false;
            }
        }

        private void showLogin()
        {
            var dlg = new LoginForm();
            if (dlg.ShowDialog(this) != DialogResult.OK)
            {
                this.WindowState = FormWindowState.Minimized;
                this.ShowInTaskbar = false;
                return;
            }

            this.session_ = this.api_.login_begin(dlg.login, dlg.password);
            this.timer1.Enabled = true;
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            this.api_ = new vds_api();
            try
            {
                this.api_.start();
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, this.Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
                Application.Exit();
            }

            if (!this.api_.local_storage_exists())
            {
                trayNotifyIcon.ShowBalloonTip(1000, this.Text, "Не зарегистрированы хранилища", ToolTipIcon.Info);
            }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (this.session_ != null && !this.is_authorized_)
            {
                try {
                    if (!this.session_.check_auth())
                    {
                        return;
                    }

                    toolStripStatusLabel1.Text = "Загрузка хранилищ...";
                    this.is_authorized_ = true;
                    addButton.Enabled = true;
                    loadStorages();
                }
                catch (Exception ex)
                {
                    this.ensureVisible();

                    MessageBox.Show(this, "Ошибка входа", this.Text, MessageBoxButtons.OK, MessageBoxIcon.Error);

                    this.session_.Dispose();
                    this.session_ = null;

                    this.showLogin();
                }
            }
        }

        private void loadStorages()
        {
            ListViewGroup currentGroup = null;
            ListViewGroup otherGroup = null;
            foreach (var storage in this.session_.get_device_storages())
            {
                if (storage.current)
                {
                    if (currentGroup == null)
                    {
                        currentGroup = new ListViewGroup("Текущий компьютер");
                        listStorage.Groups.Insert(0, currentGroup);
                    }

                    listStorage.Items.Add(toItem(storage, currentGroup));
                }
                else
                {
                    if (otherGroup == null)
                    {
                        otherGroup = new ListViewGroup("Другие компьютеры");
                        listStorage.Groups.Add(otherGroup);
                    }

                    listStorage.Items.Add(toItem(storage, otherGroup));
                }
            }

            toolStripStatusLabel1.Text = "Готово";
            if (listStorage.Items.Count == 0)
            {
                addStorage();
            }
        }

        private ListViewItem toItem(vds_api.device_storage storage, ListViewGroup group)
        {
            var item = new ListViewItem(group);
            item.SubItems[0].Text = storage.node;
            item.SubItems.Add(storage.local_path);
            item.SubItems.Add(Utils.FormatSize(storage.reserved_size));
            item.SubItems.Add(Utils.FormatSize(storage.used_size));
            return item;
        }

        private void ensureVisible()
        {
            if (this.WindowState == FormWindowState.Minimized)
            {
                this.WindowState = FormWindowState.Normal;
                this.ShowInTaskbar = true;
            }
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void cancleToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (this.session_ != null && !this.is_authorized_)
            {
                this.session_.Dispose();
                this.session_ = null;

                this.showLogin();
            }
        }

        private void addButton_Click(object sender, EventArgs e)
        {
            addStorage();
        }

        private void addStorage()
        {
            var dlg = new StorageForm();
            dlg.Api = this.api_;
            dlg.Session = this.session_;
            if (dlg.ShowDialog(this) == DialogResult.OK)
            {
                listStorage.Items.Clear();
                listStorage.Groups.Clear();
                loadStorages();
            }
        }

        private void trayNotifyIcon_BalloonTipClicked(object sender, EventArgs e)
        {
            ensureVisible();
            showLogin();
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (e.CloseReason == CloseReason.UserClosing)
            {
                this.WindowState = FormWindowState.Minimized;
                this.ShowInTaskbar = false;
                e.Cancel = true;
            }
        }
    }
}
