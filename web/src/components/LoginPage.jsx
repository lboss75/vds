import React from 'react';
import { connect } from 'react-redux';
import { withStyles } from '@material-ui/core/styles';
import { bindActionCreators } from 'redux';
import { actionCreators } from "../store/vds_api";
import { Input, Button, Grid, Link, CircularProgress } from '@material-ui/core';

const styles = theme => ({
    root: {
        flexGrow: 1,
      }
});

class LoginPage extends React.Component{
    state = {
        userEmail: '',
        userPassword: ''
      };
    
    handleLogin = () => {
        this.props.login(this.userEmail, this.userPassword);
    }

    render() {
        const { classes, theme } = this.props;
        const loading = (this.props.vdsApiState == 'login');
        return (
            <Grid container spacing={2}>
                <Grid item xs={12}>
                    <Grid container justify="center" spacing={2}>
                        <Grid item>
                            <h1>Вход в систему</h1>
                            <p>Укажите адрес электронной почты и пароль, указанные при регистрации:</p>
                            <Input placeholder="адрес электронной почты" /><br/>
                            <Input type="password" placeholder="пароль" />
                        </Grid>
                    </Grid>
                </Grid>
                <Grid item xs={12}>
                    <Grid container justify="center" spacing={2}>
                        <Grid item>
                            {loading && <CircularProgress  className={classes.fabProgress} />}
                            <Button onClick={this.handleLogin}>Войти</Button>
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
  
