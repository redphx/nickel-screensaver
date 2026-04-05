# Nickel Screensaver

Nickel Screensaver is an addon that brings the transparent screensaver feature to Kobo eReaders, similar to the one on KOReader.

<table>
  <tbody>
    <tr>
      <td>Transparent overlay + book screenshot</td>
      <td>Transparent overlay + book cover</td>
      <td>Transparent overlay + wallpaper</td>
    </tr>
    <tr>
      <td><img height="600" src="https://github.com/user-attachments/assets/5c243395-efae-4d15-9b3c-3a092723015a"></td>
      <td><img height="600" src="https://github.com/user-attachments/assets/f1470098-1210-4dc3-b8ab-6ef31f875f81"></td>
      <td><img height="600" src="https://github.com/user-attachments/assets/1f38b2a3-ebf4-45e2-8ecb-66836ad44b4c"></td>
    </tr>
  </tbody>
</table>

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

Make sure to read the `Preparation` section first. After enabling the Kobo screensaver feature, follow these steps to install:  

1. Connect your Kobo eReader to your computer
2. Download the latest [KoboRoot.tgz file](https://github.com/redphx/nickel-screensaver/releases/latest) and place it inside the hidden `.kobo` folder on your Kobo eReader
3. Eject the device safely

After it installs and reboots, try to lock the screen. If it shows your screensaver, that means it works. If it doesn't, check the **Troubleshooting** section.  

> [!IMPORTANT]
> If screensaver images appear in your library, see the **Troubleshooting** section for the fix.

The file structure before and after installation:  

<table>
  <thead>
    <tr>
      <td><b>Before</b></td>
      <td><b>After</b></td>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>
<pre>
.kobo/
├─ screensaver/
│  ├─ cat.png
│  ├─ dog.jpg
├─ KoboRoot.tgz
&nbsp;
&nbsp;
&nbsp;
&nbsp;
&nbsp;
</pre>
      </td>
      <td>
<pre>
.adds/
├─ screensaver/
│  ├─ wallpaper/
│  │  ├─ overlay/
│  │  ├─ cover
│  ├─ _settings.ini
│  ├─ cat.png
│  ├─ dog.jpg
.kobo/
├─ screensaver/
</pre>
      </td>
    </tr>
  </tbody>
</table>  

From now on, `.adds/screensaver/` is the new location for your screensavers. Don't place them in `.kobo/screensaver` anymore (but you still need to keep this folder).  

One more note: don't unlock the device immediately while it's still in the locking procress, as that may cause a crash and reboot.

# How to use

<table>
  <tbody>
    <tr>
      <td><img height="600" src="https://github.com/user-attachments/assets/19bf0d5d-0712-49bd-a012-a964d582af5b" /></td>
      <td>
        Screensaver images created by Nickel Screensaver have three layers:
        <br><br>
        <ul>
          <li>[1] Overlay layer</li>
          <li>[2] Color layer (defined in the <b>_settings.ini</b> file)</li>
          <li>[3] Image layer</li>
        </ul>
      </td>
    </tr>
  </tbody>
</table>

|     | While reading | Other screens |
|-----|---------------|---------------|
| [1] | a random image in `.adds/screensaver` | a random image in `.adds/screensaver/wallpaper/overlay` |
| [2] | `Book/ColorOverlay` and `Book/ColorOverlayAlpha` settings | `Wallpaper/ColorOverlay` and `Wallpaper/ColorOverlayAlpha` settings |
| [3] | current page screenshot | a random image in `.adds/screensaver/wallpaper` |

Nickel Screensaver only supports PNG and JPG formats. Make sure your images are small and optimized. Check the **Screenshot preparation** section for more information.

### Settings file

You can change some settings by editing the `.adds/screensaver/_settings.ini` file.

```ini
[Book]
; Color of the Color overlay layer
; In Hex format (default: FFFFFF)
ColorOverlay=ffffff
; Opacity of the Color overlay layer
; Value ranges from 0 to 100 (default: 0)
; Set to 0 to disable the layer
ColorOverlayAlpha=0

[Wallpaper]
; Same as Book.ColorOverlay setting
ColorOverlay=ffffff
; Same as Book.ColorOverlayAlpha setting
ColorOverlayAlpha=0

[Glitch]
; Enable/disable the glitching effect on book page
; Value: true/false
Enabled=false
; The number of iterations
; Value ranges from 2 to 10 (default: 5, lower is faster)
Iterations=5
; Quality of the glitched image
; Value ranges from 10 to 100 (default: 10, lower is faster)
Quality=10
```

# FAQs
1. **Can I still use NickelMenu to toggle the Screensaver feature?**  
Yes! Nickel Screensaver won't run when the `.kobo/screensaver` folder is missing.

2. **How many screenshots/screensavers does it keep?**  
~~It only keeps one screenshot/screensaver. Whenever you lock the device, it always overwrites the `.kobo/screensaver/nickel-screensaver.jpg` file.~~  
Since version 1.2, Nickel Screensaver no longer writes screensavers into disk. Now it displays them directly from memory.

# Screensaver/Wallpaper preparation  

## Best practices:  
1. Images must be in either PNG or JPG format, with `.png` and `.jpg` extensions (case-sensitive)
2. A file size under 1 MB is recommended. You can use services like [Squoosh](https://squoosh.app/) to reduce the file size.
3. To avoid unnecessary slowdown, the image dimensions must exactly match your Kobo eReader's screen resolution (for example, it's must be 1072x1448px for Kobo Clara BW). If it doesn't match, Nickel Screensaver will take extra time to scale the unoptimized image first (it does that every time).
    > You can check your device's resolution using [comparisontabl.es](https://comparisontabl.es/kobo-e-readers/) (don't forget to swap the dimensions, e.g., 1448x1072 to 1072x1448)

## Resources

- Pre-made screensaver/wallpaper:
  - [redphx/ereader-screensaver](https://github.com/redphx/ereader-screensaver)
  - [readerbackdrop.com](https://www.readerbackdrop.com/explore?tag=png)
  - [ereader-related subreddits](https://old.reddit.com/r/ereader+kobo+kindle+koreader/search?q=transparent+screensaver&restrict_sr=on&include_over_18=on&sort=relevance&t=all)
- Wallpapers:
  - [unsplash.com](https://unsplash.com/t/wallpapers)
  - [pexels.com](https://pexels.com)
- Graphic assets:
  - [huaban.com](https://huaban.com)
  - [nicepng.com](https://nicepng.com)
  - [cleanpng.com](https://cleanpng.com)
  - [stickpng.com](https://www.stickpng.com)
  - [pngall.com](https://www.pngall.com)


# How to disable or uninstall  
To temporary disable Nickel Screensaver, simply turn off the Kobo screensaver feature, or rename the `.kobo/screensaver` folder to something else (you can do this with [NickelMenu](https://github.com/pgaskin/NickelMenu)).  

To uninstall, put a file named `uninstall.txt` in the `.adds/screensaver` folder, then reboot the device.

<pre>
.adds/
├─ screensaver/
│  ├─ uninstall.txt
</pre>

# Troubleshooting  

### 1. The device doesn't sleep (or reboots) when pressing the Power button  

That means there is a bug with Nickel Screensaver. Don't panic, just uninstall it, then report the bug with your device model & firmware version.

### 2. Screensaver images appear in my library  

You need to edit Kobo's setting file to prevent it from scanning hidden folders.  

1. Connect your Kobo eReader to your computer
2. Open the `.kobo/Kobo/Kobo eReader.conf` file with a text editor
3. In the `[FeatureSettings]` section, replace the line that starts with `ExcludeSyncFolders=` with the following (insert it if not found):
  ```
  ExcludeSyncFolders=(\\.(?!kobo|adobe).+|([^.][^/]*/)+\\..+)
  ```
4. Save and eject the device safely
5. Reboot the device

# Build from source
To build Nickel Screensaver, run the following command:  

```
docker run --volume="$PWD:$PWD" --user="$(id -u):$(id -g)" --workdir="$PWD" --env=HOME --entrypoint=make --rm -it ghcr.io/pgaskin/nickeltc:1.0 all koboroot
```

# Acknowledgements

- Thanks to **pgaskin** for his [NickelHook](https://github.com/pgaskin/NickelHook) project
- Thanks to the creators of these projects for their sample code: [shermp/NickelClock](https://github.com/shermp/NickelClock), [tsowell/kobo-btpt](https://github.com/tsowell/kobo-btpt)  

# ✨ Other Kobo projects from me

- [Chokobo](https://github.com/redphx/chokobo): setup your own free, personal, private utility to convert epub books to kepub on Dropbox for Kobo e-readers (alternative to [send.djazz.se](https://send.djazz.se))
- [Kobo Tweaks](https://github.com/redphx/kobo-tweaks): a beginner-friendly addon for customizing Kobo eReaders.

<center>
  <img height="500" alt="image" src="https://github.com/redphx/kobo-tweaks/raw/main/resources/screenshots/demo-before.png" /><img height="500" alt="image" src="https://github.com/redphx/kobo-tweaks/raw/main/resources/screenshots/demo-after.png" />
</center>
