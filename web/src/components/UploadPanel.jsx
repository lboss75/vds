import React from 'react';
import { connect } from 'react-redux';
import { withStyles } from '@material-ui/core/styles';
import PropTypes from 'prop-types';
import { bindActionCreators } from 'redux';
import { actionCreators } from "../store/vds_api";

const styles = theme => ({
});

class UploadPanel extends React.Component {
    constructor(props) {
        super(props);
        this.body = props.children;
    }

    render() {
        const { classes, theme } = this.props;
        
        return (
            <div>
                <input type="file" id="files" name="files[]" multiple />
            </div>
        );
    }
}
  
UploadPanel.propTypes = {
    classes: PropTypes.object.isRequired,
    theme: PropTypes.object.isRequired,
  };
  
  export default connect(
    state => state.vdsApi,
    dispatch => bindActionCreators(actionCreators, dispatch)
  )(withStyles(styles, { withTheme: true })(UploadPanel));
  