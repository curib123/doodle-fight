param(
    [string]$PackageName = "org.cocos2dx.cpp"
)

$ErrorActionPreference = "Stop"

if (-not (Get-Command adb -ErrorAction SilentlyContinue)) {
    throw "adb was not found. Install Android Platform Tools and add adb to PATH."
}

$devices = adb devices
if ($devices -notmatch "\tdevice") {
    throw "No authorized Android device is connected."
}

$sdk = [int](adb shell getprop ro.build.version.sdk).Trim()
Write-Host "Android API level: $sdk"
Write-Host "Package: $PackageName"

adb shell pm list packages $PackageName | Out-Null

if ($sdk -ge 33) {
    Write-Host "Granting NEARBY_WIFI_DEVICES when declared..."
    adb shell pm grant $PackageName android.permission.NEARBY_WIFI_DEVICES 2>$null
}

if ($sdk -ge 36) {
    Write-Host "Enabling Android local-network restriction compatibility test..."
    adb shell am compat enable RESTRICT_LOCAL_NETWORK $PackageName
}

if ($sdk -ge 37) {
    Write-Host "Granting ACCESS_LOCAL_NETWORK when declared..."
    adb shell pm grant $PackageName android.permission.ACCESS_LOCAL_NETWORK 2>$null
}

Write-Host ""
Write-Host "LAN test preparation complete."
Write-Host "1. Launch Doodle Fight on two devices on the same Wi-Fi."
Write-Host "2. Host on one device and verify UDP room discovery on the other."
Write-Host "3. Verify movement/fire traffic in both directions."
Write-Host "4. Revoke Nearby devices/local-network permission and confirm discovery fails gracefully."
Write-Host "5. Re-grant permission and confirm discovery recovers."
Write-Host ""
Write-Host "To remove the Android 16 compatibility test flag:"
Write-Host "adb shell am compat disable RESTRICT_LOCAL_NETWORK $PackageName"
