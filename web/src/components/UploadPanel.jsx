import React from 'react';
import { connect } from 'react-redux';
import { withStyles } from '@material-ui/core/styles';
import PropTypes from 'prop-types';
import { bindActionCreators } from 'redux';
import { actionCreators } from "../store/vds_api";
import { Typography, LinearProgress, Snackbar, SnackbarContent, Icon } from '@material-ui/core';

const styles = theme => ({
    error: {
        backgroundColor: theme.palette.error.dark,
    },
    iconVariant: {
        opacity: 0.9
    },
});

class UploadPanel extends React.Component {
    state = {
        showError: true
    };

    constructor(props) {
        super(props);
        this.body = props.children;
    }

    handleFileChange = async (event) => {
        event.persist();
        this.setState({showError:true});
        await this.props.upload(event.target.files);
    }

    handleClose = () => {
        this.setState({showError:false});
    }


    render() {
        const { classes, theme } = this.props;
        const { showError } = this.state;
        const { vdsApiUploading, vdsApiUploaded, vdsApiUploadSize} = this.props;

        const loading = (this.props.vdsApiUploading !== '');
        const progress = (0 < vdsApiUploadSize) ? (100 * vdsApiUploaded / vdsApiUploadSize) : 0;
        const showSnackbar = (showError && this.props.vdsApiUploadError !== '');

        return (
            <div>
                <input type="file" id="files" name="files[]" multiple onChange={this.handleFileChange} />
                {loading && <Typography variant="subtitle2" gutterBottom>{vdsApiUploading}</Typography>}
                {loading && <LinearProgress value={progress} />}                
                <Snackbar
                    anchorOrigin={{
                    vertical: 'bottom',
                    horizontal: 'left',
                    }}
                    open={showSnackbar}
                    autoHideDuration={6000}
                    onClose={this.handleClose}
                >
                    <SnackbarContent
                    className={classes.error}
                    aria-describedby="client-snackbar"
                    message={
                        <span id="client-snackbar" className={classes.message}>
                        <Icon className={classes.iconVariant} />
                        {this.props.vdsApiUploadError}
                        </span>
                    }
                    />
                </Snackbar>

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
  