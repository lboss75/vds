import React from 'react';
import { connect } from 'react-redux';
import { withStyles } from '@material-ui/core/styles';
import { bindActionCreators } from 'redux';
import { actionCreators } from "../../store/vds_api";
import { TextField, Button, Grid, Link, CircularProgress, Snackbar, SnackbarContent, Icon } from '@material-ui/core';

const styles = theme => ({
});

class StorageCreate extends React.Component{
    state = {
        storagePath: '',
        storageSize: 1024 * 1024 * 1024,
        showError: true
      };
    
    handleCreate = async () => {
        this.setState({showError:true});
        try{
            await this.props.create_storage(this.state.storagePath, this.state.storageSize);
            this.props.history.push('/app');
        }
        catch(ex){
            this.setState({showError:true, Error: ex});
        }
    }

    handleClose = () => {
        this.setState({showError:false});
    }

     handleChange = (name) => event => {
        this.setState({ [name]: event.target.value });
    };

    render() {
        const { classes } = this.props;
        const { showError } = this.state;
        const loading = (this.props.vdsApiState === 'login');
        const loginError = this.props.vdsApiLoginError;
        const showSnackbar = (showError && loginError !== '');

        return (
            <h1>Выделение дискового пространства</h1>
            <p>Укажите папку для хранения файлов. Для подтверждения ваших прав
                на данную папку скачайте файл по данной ссылке и поместите его в эту папку</p>
            <TextField
                id="standard-required"
                placeholder="Папка для хранения файлов"
                value={this.state.storagePath}
                onChange={this.handleChange('storagePath')}
            /><br/>
            <TextField
                placeholder="Выделеный размер"
                value={this.state.storageSize}
                onChange={this.handleChange('storageSize')}
            />
            {loading && <CircularProgress  className={classes.fabProgress} />}
            <Button onClick={this.handleCreate}>Создать</Button>

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
                    {this.props.vdsApiLoginError}
                    </span>
                }
                />
            </Snackbar>
        );
    }
}
export default connect(
    state => state.vdsApi,
    dispatch => bindActionCreators(actionCreators, dispatch)
  )(withStyles(styles, { withTheme: true })(StorageCreate));
  
