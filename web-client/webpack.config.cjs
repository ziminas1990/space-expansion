const path = require('path');

module.exports = {
  entry: './dist/webapp/main.js',
  output: {
    filename: 'main.js',
    path: path.resolve(__dirname, './server/static'),
  },
};

console.log(JSON.stringify(module.exports, null, 2));