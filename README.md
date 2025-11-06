# Nickel Screensaver

Nickel Screensaver is an addon that brings the transparent screensaver feature to Kobo OS, similar to the one on KOReader.

<table>
  <tbody>
    <td><img height="600" src="https://github.com/user-attachments/assets/a825e165-80fa-4dc0-9e7c-946ebd68e58d"></td>
    <td><img height="600" src="https://github.com/user-attachments/assets/2c102e2b-d70a-4128-b0a7-2c796f368b5f"></td>
  </tbody>
</table>

# How it works
Nickel Screensaver supports both PNG and JPG screensavers. It only applies the transparent effect when you're reading a book. In other screens, it displays your screensaver on top of a white screen.  

It achieves the "transparent" effect by capturing screenshot of the current screen before the device goes to sleep, then combines that with your transparent screensaver on top. These extra steps add slight delay, so make sure your PNG screensavers are small and optimized. Check the **Screenshot preparation** section for more information.

# Preparation  

### 1. Make sure the Kobo screensaver feature is enabled and working  

If you haven't enabled it, follow these steps:
  1. Connect your Kobo eReader to your computer
  2. On the KOBOeReader disk, find the hidden `.kobo` folder and create a new folder named `screensaver`
  3. Put some small PNG/JPEG photos inside that folder (extensions must be either ".png" or ".jpg")
  4. Eject the device safely
  5. On your Kobo eReader, go to `Settings > Energy saving and privacy` and turn on `Show book covers full screen`
  6. Lock the device. You should see a random screensaver.

### 2. Backup your screensavers in `.kobo/screensaver`

Every time you lock the device, files in the `.kobo/screensaver` folder that aren't related to Nickel Screensaver will be moved automatically to `.adds/screensaver`.  

Usually, you don't need to do this step, but it's better safe than sorry.

# How to install

**⚠️ Requirement:** Nickel Screensaver only supports Kobo eReader running firmware 4.21.15015 and later

Make sure to read the `Preparation` section first.  
After enabling the Kobo screensaver feature, follow these steps to install:  

1. Connect your Kobo eReader to your computer
2. Download the latest [KoboRoot.tgz file](https://github.com/redphx/nickel-screensaver/releases/latest) and place it inside the hidden `.kobo` folder on your Kobo eReader
3. Eject the device safely

The file structure should look like this:  
```
.kobo/
├─ screensaver/
│  ├─ cat.png
│  ├─ dog.jpg
├─ KoboRoot.tgz
```

After it installs and reboots, try to lock the screen. If it shows your screensaver, that means it works. If it doesn't, check the **Troubleshooting** section.

Now the file structure should look like this:  
```
.adds/
├─ screensaver/
│  ├─ cat.png
│  ├─ dog.jpg
.kobo/
├─ screensaver/
```

From now on, `.adds/screensaver/` is the new location for your screensavers. Don't place them in `.kobo/screensaver` anymore.  

One more note: don't unlock the device immediately while it's still in the locking procress, as that may cause a crash and reboot.

# Screensaver preparation  

## Best practices:  
1. Screensavers must be either PNG or JPG format
2. A file size under 1MB is recommended. You can use services like [Squoosh](https://squoosh.app/) to reduce the file size.
3. To avoid unnecessary slowdown, the screensaver dimensions must exactly match your Kobo eReader's screen resolution (for example, it's must be 1072x1448px for Kobo Clara BW). If it doesn't match, Nickel Screensaver will take extra time to scale the unoptimized image first (it does that everytime).

## Resources for screensavers or for creating screensavers
- [redphx/ereader-screensaver](https://github.com/redphx/ereader-screensaver)
- [readerbackdrop.com](https://www.readerbackdrop.com/)
- [huaban.com](https://huaban.com)

# How to disable or uninstall  
To temporary disable Nickel Screensaver, simply turn off the Kobo screensaver feature, or rename the `.kobo/screensaver` folder to something else (you can do this with [NickelMenu](https://github.com/pgaskin/NickelMenu)).  

To uninstall, put a file named `uninstall` in the `.adds/screensaver` folder, then reboot the device.

# Troubleshooting  

### 1. The device doesn't sleep (or reboots) when pressing the Power button  

That means there is a bug with Nickel Screensaver. Don't panic, just uninstall it, then report the bug with your device model & firmware version.

### 2. Screensaver images show up in my library  

You need to edit Kobo's setting file to prevent it from scanning hidden folders.  

1. Connect your Kobo eReader to your computer
2. Open the `.kobo/Kobo/Kobo eReader.conf` file with a text editor
3. Find the line that starts with `ExcludeSyncFolders=`, then replace the entire line with this:
  ```
  ExcludeSyncFolders=(\\.(?!kobo|adobe).+|([^.][^/]*/)+\\..+)
  ```
4. Save and eject the device safely
5. Reboot the device
