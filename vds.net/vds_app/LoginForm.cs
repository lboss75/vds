using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace vds_app
{
    public partial class LoginForm : Form
    {
        public LoginForm()
        {
            InitializeComponent();
        }

        private void cancelBtn_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void okBtn_Click(object sender, EventArgs e)
        {
            var api = new vds_api();
            var task = api.login(loginEdit.Text, passwordEdit.Text);
            var dlg = new ProgressForm();
            dlg.Task = task;
            if (dlg.ShowDialog(this) == DialogResult.OK)
            {
                DialogResult = DialogResult.OK;
            }
        }
    }
}
