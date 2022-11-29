<?PHP

$db = array(
    "80:7D:3A:79:01:7B" => "LetiClock-v11"
);

header('Content-type: text/plain; charset=utf8', true);
$headers = getallheaders();

function check_header($name, $value = false)
{
    global $headers;
    if (!isset($headers[$name])) {
        return false;
    }
    if ($value && $headers[$name] != $value) {
        return false;
    }
    return true;
}

function sendFile($path)
{
    header($_SERVER["SERVER_PROTOCOL"] . ' 200 OK', true, 200);
    header('Content-Type: application/octet-stream', true);
    header('Content-Disposition: attachment; filename=' . basename($path));
    header('Content-Length: ' . filesize($path), true);
    header('x-MD5: ' . md5_file($path), true);
    readfile($path);
}

if (!check_header('User-Agent', 'ESP8266-http-Update')) {
    header($_SERVER["SERVER_PROTOCOL"] . ' 403 Forbidden', true, 403);
    echo "only for ESP8266 updater!\n";
    exit();
}

if (
    !check_header('x-ESP8266-STA-MAC') ||
    !check_header('x-ESP8266-AP-MAC') ||
    !check_header('x-ESP8266-free-space') ||
    !check_header('x-ESP8266-sketch-size') ||
    !check_header('x-ESP8266-sketch-md5') ||
    !check_header('x-ESP8266-chip-size') ||
    !check_header('x-ESP8266-sdk-version')
) {
    header($_SERVER["SERVER_PROTOCOL"] . ' 403 Forbidden', true, 403);
    echo "only for ESP8266 updater! (header)\n";
    exit();
}

if (!isset($db[$headers['x-ESP8266-STA-MAC']])) {
    $message = $_SERVER["SERVER_PROTOCOL"] . ' 500 ESP MAC not configured for updates';
    error_log($message);
    header($message, true, 500);
}

$localBinary = "/firmware/" . $db[$headers['x-ESP8266-STA-MAC']] . ".bin";

// Check if version has been set and does not match, if not, check if
// MD5 hash between local binary and ESP8266 binary do not match if not.
// then no update has been found.
/**
 * @param $xESPsketchmd
 * @param $localBinary
 * @return bool
 */
function doesFirmwareHashMatch($xESPsketchmd, $localBinary)
{
    $file_md5 = md5_file($localBinary);
    $b = $xESPsketchmd == $file_md5;
    return $b;
}

/**
 * @param $versionFromDb
 * @param $versionFromHeaders
 * @return bool
 */
function isNewerVersionAvailable($versionFromDb, $versionFromHeaders)
{

    $b = ((int) substr($versionFromDb, 11)) > ((int) substr($versionFromHeaders, 11));
    return $b;
}

error_log("Chip " . $headers["x-ESP8266-STA-MAC"] . " with firmware: " . $headers['x-ESP8266-version'] . " asking for update");


if (isNewerVersionAvailable($db[$headers['x-ESP8266-STA-MAC']], $headers['x-ESP8266-version'])) {
    error_log('Sending binary ' . $localBinary);
    sendFile($localBinary);
} else {
    error_log('No update required');
    header($_SERVER["SERVER_PROTOCOL"] . ' 304 Not Modified', true, 304);
}
