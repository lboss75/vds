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

    getFileFromInput(file) {
        return new Promise(function (resolve, reject) {
            const reader = new FileReader();
            reader.onerror = reject;
            reader.onload = function () { resolve(reader.result); };
            reader.readAsBinaryString(file); // here the file can be read in different way Text, DataUrl, ArrayBuffer
        });
    }

    handleFileChange = async (event) => {
        event.persist();
        await this.props.upload(event.target.files);
    }

    render() {
        const { classes, theme } = this.props;
        
        return (
            <div>
                <input type="file" id="files" name="files[]" multiple onChange={this.handleFileChange} />
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
  