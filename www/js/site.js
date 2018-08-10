function humanFileSize(bytes, si) {
    var thresh = si ? 1000 : 1024;
    if (Math.abs(bytes) < thresh) {
        return bytes + ' байт';
    }
    var units = si
        ? ['kB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB']
        : ['KiB', 'MiB', 'GiB', 'TiB', 'PiB', 'EiB', 'ZiB', 'YiB'];
    var u = -1;
    do {
        bytes /= thresh;
        ++u;
    } while (Math.abs(bytes) >= thresh && u < units.length - 1);
    return bytes.toFixed(1) + ' ' + units[u];
}

function load_channels(session_id) {
    $.getJSON('api/channels?session=' + session_id,
        function(data) {
            $.each(data,
                function() {
                    $('#channelsMenu')
                        .append($('<li/>')
                            .append($('<a class="selectChannelMenu" href="#" data-menu="' +
                                    this.object_id +
                                    '">' +
                                    this.name +
                                    '</a>')
                                .on('click',
                                    function(e) {
                                        $this = $(e.currentTarget);
                                        $('#balance_panel').hide();
                                        $('#chat_feed').show();

                                        channel_id = $this.data('menu');
                                        load_channel(channel_id, $this.text());
                                        $('#channel_id').val(channel_id);
                                    })));
                });
        });
}

function after_login(auth_state) {
    $('#login_menu').hide();
    $('#user_menu').show();
    $('#user_login').text(auth_state.user_name);
    $('#anonymous_content').hide();
    $('#user_content').show();

    load_channels(auth_state.session);
}

function try_login() {
    $.ajax({
        url: 'api/try_login?login=' +
            encodeURIComponent(inputEmail.value) +
            '&password=' +
            encodeURIComponent(inputPassword.value),
        success: function(data) {
            if ('sucessful' == data.state) {
                $('#processDialog').modal('hide');
                //window.location.href = data.url;
                sessionStorage.setItem("vds_auth", JSON.stringify(data));
                after_login(data);
            } else {
                $('#progressBar').css('width', data.state + '%').attr('aria-valuenow', data.state);
                try_login();
            }
        }
    });
}

