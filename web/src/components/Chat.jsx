import React from 'react';
import { connect } from 'react-redux';
import { withStyles } from '@material-ui/core/styles';
import UploadPanel from './UploadPanel';
import PropTypes from 'prop-types';
import { bindActionCreators } from 'redux';
import { actionCreators } from '../store/vds_api';

const styles = theme => ({
});

class Chat extends React.Component {
    constructor(props) {
        super(props);
        this.body = props.children;
    }

    render() {
        const { classes, theme } = this.props;
        
        return (
            <div>
                <UploadPanel />
            </div>
        );
    }
}
  
Chat.propTypes = {
    classes: PropTypes.object.isRequired,
    theme: PropTypes.object.isRequired,
};

export default connect(
)(withStyles(styles, { withTheme: true })(Chat));
