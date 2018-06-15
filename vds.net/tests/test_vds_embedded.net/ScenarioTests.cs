using System;
using Xunit;
using vds_embedded.net;
using System.IO;

namespace test_vds_embedded.net
{
    public class ScenarioTests
    {
        [Fact]
        public void ScenarionTests()
        {
            var api = new vds_api();
            api.root_folder(Path.Combine(Directory.GetCurrentDirectory(), "servers"));
            api.server_root("vadim@iv-soft.ru", "123");
        }
    }
}
