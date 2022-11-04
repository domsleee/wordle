
const filename = process.argv[2];
const fs = require('fs')
fs.readFile(filename, 'utf8', function (err,data) {
  if (err) {
    return console.log(err);
  }
  data = data
    .replace(/⬛️/g, '_')
    .replace(/🟨/g, '?')
    .replace(/🟩/g, '+')
    .replace(/"map"/g, '"next"');

  const matcher = /("\d+")/g;
  data = data.replace(matcher, m => {
    return m
      .replace(/0/g, '_')
      .replace(/1/g, '?')
      .replace(/2/g, '+');
  });
  console.log(data);
});