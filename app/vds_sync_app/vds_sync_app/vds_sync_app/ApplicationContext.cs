using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;
using Xamarin.Forms.Internals;

namespace vds_sync_app
{
    public class ApplicationContext
    {
        public ObservableCollection<UserChannel> Channels { get; set; } = new ObservableCollection<UserChannel>();
    }
}
