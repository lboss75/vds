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

function load_channel(session_id, channel_id, channel_name) {
    $.ajax({
        url: '/api/channel_feed?session='
            + encodeURIComponent(session_id)
            + "&channel_id="
            + encodeURIComponent(channel_id),
        success: function (data) {
            $('#feed_records').empty();
            $.each(data,
                function() {
                    var files = $('<ul class="list-group" />'); 
                        $.each(this.files,
                        function() {
                            files
                                .append(
                                    $('<li class="list-group-item" />')
                                    .append($('<a />')
                                            .attr('href', '/api/download?session='
                                            + encodeURIComponent(session_id)
                                            + "&channel_id="
                                            + encodeURIComponent(channel_id)
                                            + "&object_id="
                                            + encodeURIComponent(this.object_id))
                                            .text(this.name))
                                    .append($('<span class="badge"/>')
                                        .text(humanFileSize(this.size, 1000))));
                        });

                        $('#feed_records')
                            .append($('<div class="panel panel-default" />')
                                .append($('<div class="panel-heading" />')
                                    .text(this.message)
                                    .append(files)));
                }
            );
        }
    });
}

function load_channels(session_id) {
    $.getJSON('api/channels?session=' + session_id,
        function(data) {
            $('#channelsMenu').empty();
            $.each(data,
                function () {
                    var current_channel = sessionStorage.getItem("vds_channel");
                    if (!current_channel) {
                        current_channel = this.object_id;
                        sessionStorage.setItem("vds_channel", current_channel);
                    }
                    $('#channelsMenu')
                        .append($((current_channel == this.object_id) ? '<li class="nav-item  active"/>' : '<li class="nav-item"/>')
                            .append($('<a class="nav-link" href="#" data-menu="' +
                                    this.object_id +
                                    '">' +
                                    this.name +
                                    '</a>')
                                .on('click',
                                    function(e) {
                                        $this = $(e.currentTarget);

                                        channel_id = $this.data('menu');
                                        sessionStorage.setItem("vds_channel", channel_id);
                                        $('#channel_id').val(channel_id);
                                        load_channels(session_id);
                                    })));
                    if (current_channel == this.object_id) {
                        load_channel(session_id, this.object_id, this.name);
                    }
                });
        });
}

function after_login(auth_state) {
    $('#login_menu').hide();
    $('#user_menu').show();
    $('#user_login').text(auth_state.user_name);
    $('#anonymous_content').hide();
    $('#user_content').show();
    $('#logoutForm').attr('action', '/api/logout?session=' + auth_state.session);
    $('#send_message').attr('action', 'upload?session=' + auth_state.session);
    load_channels(auth_state.session);
}

function after_logout() {
    $('#user_menu').hide();
    $('#login_menu').show();
    $('#user_content').hide();
    $('#anonymous_content').show();
    $('#login_btn').on('click',
        function() {
            $('#processDialog').modal('show');
            $('#loginModal').modal('hide');
            try_login();
            return false;
        });
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
                sessionStorage.setItem("vds_session", data.session);
                after_login(data);
            } else {
                $('#progressBar').css('width', data.state + '%').attr('aria-valuenow', data.state);
                try_login();
            }
        }
    });
}

