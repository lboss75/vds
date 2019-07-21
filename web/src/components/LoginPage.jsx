import React from 'react';
import { connect } from 'react-redux';
import { withStyles } from '@material-ui/core/styles';
import { bindActionCreators } from 'redux';
import { actionCreators } from "../store/vds_api";
import { TextField, Button, Grid, Link, CircularProgress, Snackbar, SnackbarContent, Icon } from '@material-ui/core';

const styles = theme => ({
    root: {
        flexGrow: 1,
    },
    error: {
        backgroundColor: theme.palette.error.dark,
    },
    iconVariant: {
        opacity: 0.9
    },
});

class LoginPage extends React.Component{
    state = {
        userEmail: '',
        userPassword: '',
        showError: true
      };
    
    handleLogin = () => {
        this.setState({showError:true});
        this.props.login(this.userEmail, this.userPassword);
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
        const loading = (this.props.vdsApiState == 'login');
        const loginError = this.props.vdsApiLoginError;
        const showSnackbar = (showError && loginError != '');

        return (
            <Grid container spacing={2}>
                <Grid item xs={12}>
                    <Grid container justify="center">
                        <Grid item>
                            <h1>Вход в систему</h1>
                            <p>Укажите адрес электронной почты и пароль, указанные при регистрации:</p>
                            <TextField
                                id="standard-required"
                                placeholder="адрес электронной почты"
                                value={this.state.userEmail}
                                onChange={this.handleChange('userEmail')}
                            /><br/>
                            <TextField
                                type="password"
                                placeholder="пароль"
                                value={this.state.userPassword}
                                onChange={this.handleChange('userPassword')}
 />
                        </Grid>
                    </Grid>
                </Grid>
                <Grid item xs={12}>
                    <Grid container justify="center">
                        <Grid item>
                            {loading && <CircularProgress  className={classes.fabProgress} />}
                            <Button onClick={this.handleLogin}>Войти</Button>

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

                        </Grid>
                    </Grid>
                </Grid>
                <Grid item xs={12}>
                    <Grid container justify="center" spacing={2}>
                        <Grid item>
                            <p>Если Вы ещё не зарегистрировались, пожалуйста <Link to='/register'>зарегистрируйтесь</Link></p>                            
                        </Grid>
                    </Grid>
                </Grid>
            </Grid>
        );
    }
}
export default connect(
    state => state.vdsApi,
    dispatch => bindActionCreators(actionCreators, dispatch)
  )(withStyles(styles, { withTheme: true })(LoginPage));
  
