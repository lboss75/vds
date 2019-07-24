import React from 'react';
import { withRouter } from 'react-router-dom';
import PropTypes from 'prop-types';
import { bindActionCreators } from 'redux';
import { actionCreators } from "../store/vds_api";
import { withStyles } from '@material-ui/core/styles';
import { connect } from 'react-redux';
import { List, ListItem, ListItemAvatar, Avatar, ListItemText, Link } from '@material-ui/core';
import { Image } from '@material-ui/icons';

const styles = theme => ({
    root: {
        width: '100%',
        maxWidth: 360,
        backgroundColor: theme.palette.background.paper,
    }
});

class AppList extends React.Component {
    constructor(props) {
      super(props);
      this.body = props.children;
    }

    render() {
        const { classes } = this.props;
        const items = this.props.vdsApiChannels;

        return (
            <List className={classes.root}>
                {items.map((item) => {
                    return (
                        <ListItem key={item.channel_id} button onClick={() => this.props.history.push(`/app/${encodeURIComponent(item.channel_id)}`)}>
                            <ListItemAvatar>
                                <Avatar>
                                    <Image />
                                </Avatar>
                            </ListItemAvatar>
                            <ListItemText primary={item.channel_name} secondary={item.channel_type} />
                        </ListItem>
                    )})}
            </List>
        )
    }
}

AppList.propTypes = {
    classes: PropTypes.object.isRequired,
    theme: PropTypes.object.isRequired,
  };
  
  export default connect(
    state => state.vdsApi,
    dispatch => bindActionCreators(actionCreators, dispatch)
  )(withStyles(styles, { withTheme: true })(withRouter(AppList)));
  