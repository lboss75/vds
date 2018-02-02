using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Xamarin.Forms;

namespace vds_sync_app
{
	public partial class MainPage : ContentPage
	{
		public MainPage()
		{
            this.BindingContext = new ApplicationContext();
		    this.BindingContext.Channels.Add(new UserChannel
		    {
                Title = "Test",
                SubTitle = "SubTitle"
		    });


            InitializeComponent();
		}

	    public ApplicationContext BindingContext
        {
	        get
	        {
	            return (ApplicationContext)base.BindingContext;
	        }

	        set
	        {
                base.BindingContext = value;
	        }
	    }

	    private void Menu_OnClicked(object sender, EventArgs e)
	    {
	    }
	}
}
