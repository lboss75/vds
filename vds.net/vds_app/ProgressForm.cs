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
    public partial class ProgressForm : Form
    {
        public ProgressForm()
        {
            InitializeComponent();
        }

        public vds_api.vds_task Task { get; set; }

        private void timer1_Tick(object sender, EventArgs e)
        {
            this.Task.update();

            status_label.Text = this.Task.status_text;
            progressBar1.Value = this.Task.percent;

            if (this.Task.state == "done")
            {
                DialogResult = DialogResult.OK;
            }
        }

        private void ProgressForm_Load(object sender, EventArgs e)
        {

        }
    }
}
