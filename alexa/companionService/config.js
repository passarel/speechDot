/**
 * @module
 * This module defines the settings that need to be configured for a new
 * environment.
 * The clientId and clientSecret are provided when you create
 * a new security profile in Login with Amazon.  
 * 
 * You will also need to specify
 * the redirect url under allowed settings as the return url that LWA
 * will call back to with the authorization code.  The authresponse endpoint
 * is setup in app.js, and should not be changed.  
 * 
 * lwaRedirectHost and lwaApiHost are setup for login with Amazon, and you should
 * not need to modify those elements.
 */
const certs_path = require('path').resolve(__dirname, '../client/certs');

var config = {
    clientId: 'amzn1.application-oa2-client.3fc410cbf66b4ba1b998f1f3cebd3b88',
    clientSecret: 'd94cc274e837cc2eed2e231b4a9ab1d3ab8b49b42c8972442a9f5d46605eddfa',
    redirectUrl: 'https://localhost:3000/authresponse',
    lwaRedirectHost: 'amazon.com',
    lwaApiHost: 'api.amazon.com',
    validateCertChain: true,
    sslKey: certs_path + '/server/node.key',
    sslCert: certs_path + '/server/node.crt',
    sslCaCert: certs_path + '/ca/ca.crt',
    products: {
        "my_device": ["123456"], // Fill in with valid device values, eg: "testdevice1": ["DSN1234", "DSN5678"]
    },
};

module.exports = config;
