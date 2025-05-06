const https = require('https');
const fs = require('fs');
const express = require('express');
const path = require('path');

const app = express();

// Serve static files from the 'public' directory
app.use(express.static(path.join(__dirname, 'public')));

// Load SSL certificate and key
const options = {
  key: fs.readFileSync('key.pem'),
  cert: fs.readFileSync('cert.pem')
};

// Create HTTPS server
const server = https.createServer(options, app);

// Start server on port 8000
server.listen(8000, () => {
  console.log('HTTPS server running on https://localhost:8000');
  console.log('Accessible on your network at https://Mac:8000');
});