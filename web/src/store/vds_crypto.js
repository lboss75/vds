import createHash from "create-hash";


/** @arg {string|Buffer} data
    @arg {string} [digest = null] - 'hex', 'binary' or 'base64'
    @return {string|Buffer} - Buffer when digest is null, or string
*/
function sha256(data, encoding) {
    return createHash("sha256")
        .update(data)
        .digest(encoding);
}


export function  user_credentials_to_key(user_email, user_password) {
    const password_hash = sha256(user_password, 'base64');
    return 'credentials:'
      + user_email.length() + "." + user_email + ","
      + password_hash.length() + "." + password_hash;
}
