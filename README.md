# Nickel Screensaver
(work in progress, please come back later)

Add the transparent screensaver feature to Kobo OS.

<table>
  <tbody>
    <td><img height="600" src="https://github.com/user-attachments/assets/23a29c98-3be9-4ac1-8a6e-470b9109a262"></td>
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
... because they will be deleted by Nickel Screensaver every time you lock the screen.  
You can move them to `.adds/screensaver`, this is the folder that Nickel Screensaver uses to load the screensavers.

# How to install
Make sure to read the `Preparation` section first.

1. Download the installation file (not available at the moment)
2. Connect your Kobo eReader to your computer  
3. In the `.kobo` folder, create a new folder named `screensaver`, then put your PNG/JPG photos into it
4. Copy `KoboRoot.tgz` (downloaded in step 1) into the `.kobo` folder
5. Eject the device safely

After it installs and reboots, try to lock the screen. If it shows your screensaver, that means it works. If it doesn't, check the **Troubleshooting** section

# How to uninstall  
To uninstall Nickel Screensaver, put a file named `uninstall` in the `.adds/screensaver` folder.

# Troubleshooting  

### 1. The device doesn't sleep (or reboots) when pressing the Power button  
That means there is a bug with Nickel Screensaver. Don't panic, just uninstall it, then report the bug with your device model & firmware version.

# Screensaver preparation  

Best practices:  
1. Screensavers must be either PNG or JPG format
2. A file size under 1MB is recommended. You can use services like [Squoosh](https://squoosh.app/) to reduce the file size.
3. To avoid unnecessary slowdown, the screensaver dimensions must exactly match your Kobo eReader's screen resolution (for example, it's 1072x1488px for Kobo Clara BW). If it doesn't match, Nickel Screensaver will take extra time to scale the image first.

When ready, copy your screensavers to the `.adds/screensaver` folder on your Kobo eReader, then safely eject it. Now try to open a book, lock the device and see if it works.
