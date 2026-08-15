const { execSync } = require('child_process');
const adb = 'C:\\Users\\kayf\\AppData\\Local\\Android\\Sdk\\platform-tools\\adb.exe';
execSync(`"${adb}" -s emulator-5554 shell screencap -p /data/local/tmp/screen.png`, { stdio: 'inherit' });
execSync(`"${adb}" -s emulator-5554 pull /data/local/tmp/screen.png "C:\\Users\\kayf\\.gemini\\antigravity\\brain\\f1df68df-c891-445b-810d-7d6abc6b99df\\running_game_screen_clean.png"`, { stdio: 'inherit' });
console.log('Successfully pulled clean screenshot!');
