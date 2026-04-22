# File Roller Integration Guide

## Quick Setup

### Option 1: Automatic Setup (Recommended)

Run the setup script to automatically configure MIME type associations:

```bash
cd fileRoller
./setup-mime-associations.sh
```

This script will:
- Copy the `.desktop` file to your applications directory
- Register File Roller with your desktop environment
- Set File Roller as the default handler for all supported file types

### Option 2: Manual Setup

#### Step 1: Install the Application

```bash
# Build and install all applications
make
sudo make install
```

#### Step 2: Configure Desktop File

Copy the desktop file to the applications directory:

```bash
mkdir -p ~/.local/share/applications
cp fileRoller/blackline-file-roller.desktop ~/.local/share/applications/
update-desktop-database ~/.local/share/applications/
```

#### Step 3: Set MIME Type Defaults

Set File Roller as the default for supported file types:

```bash
# For each MIME type you want to set
xdg-mime default blackline-file-roller.desktop image/png
xdg-mime default blackline-file-roller.desktop image/jpeg
xdg-mime default blackline-file-roller.desktop text/plain
# ... and so on for other types
```

## Usage with File Manager

Once configured, File Roller will automatically open supported files when:

### Method 1: Double-click files
- Open your file manager (blackline-fm or your default file manager)
- Navigate to a supported file (image, text, PDF, video, audio, archive)
- Double-click to open with File Roller

### Method 2: Right-click context menu
- Right-click on a supported file
- Select "Open With" → "File Roller"
- Or if set as default, just select "Open"

### Method 3: Drag and drop
- Drag a file from the file manager
- Drop it onto the File Roller window
- Or launch File Roller first, then drag files into it

### Method 4: Command line
```bash
# Open a file directly
blackline-file-roller /path/to/file.jpg

# Or from file manager terminal
./blackline-file-roller ~/Pictures/photo.png
```

## Supported File Types

| Category | Extensions | MIME Types |
|----------|-----------|-----------|
| **Images** | .png, .jpg, .jpeg, .gif, .bmp, .webp, .tiff, .svg | image/* |
| **Text** | .txt, .c, .h, .py, .js, .html, .css, .json, .xml, .sh, .md, .log | text/* |
| **PDF** | .pdf | application/pdf |
| **Video** | .mp4, .webm, .mkv, .avi, .mov, .flv, .wmv, .m4v | video/* |
| **Audio** | .mp3, .wav, .flac, .aac, .ogg, .m4a, .wma | audio/* |
| **Archives** | .zip, .tar, .gz, .7z, .rar, .xz, .bz2 | application/x-* |

## File Manager Integration

### For blackline-fm (LIDE File Manager)

The File Roller is automatically integrated with blackline-fm. When you open a file:

1. The file manager detects the file type
2. Launches File Roller with the appropriate arguments
3. File Roller automatically opens the file with the right viewer

### For Other File Managers (Nautilus, Dolphin, Thunar, etc.)

#### Nautilus (GNOME Files)
1. Right-click on a file
2. Select "Properties"
3. Open "With" tab
4. Select "File Roller"
5. Click "Set as Default" (if desired)

#### Dolphin (KDE)
1. Right-click on a file
2. Select "Open With" → "File Roller"
3. Or: Go to Settings → File Associations, search for the file type, and add File Roller

#### Thunar (Xfce)
1. Right-click on a file
2. Select "Open With Other Application"
3. Choose "File Roller"
4. Check "Use as default for this kind of file"

## Troubleshooting

### File doesn't open with File Roller

**Problem**: Double-clicking a file doesn't open File Roller

**Solution**:
1. Verify File Roller is installed: `which blackline-file-roller`
2. Check desktop file exists: `ls ~/.local/share/applications/blackline-file-roller.desktop`
3. Re-run setup script: `./setup-mime-associations.sh`
4. Refresh file manager cache: Press F5 or restart file manager

### File Roller not appearing in "Open With"

**Problem**: Right-click menu doesn't show File Roller as an option

**Solution**:
1. Update desktop database:
   ```bash
   update-desktop-database ~/.local/share/applications/
   ```
2. Rebuild MIME cache:
   ```bash
   update-mime-database ~/.local/share/mime/
   ```

### File opens with wrong application

**Problem**: Files open with a different application instead of File Roller

**Solution**:
1. Run the setup script again:
   ```bash
   ./setup-mime-associations.sh
   ```
2. Or manually reset MIME type:
   ```bash
   xdg-mime default blackline-file-roller.desktop image/png
   ```

### Desktop file validation

To check if your desktop file is valid:

```bash
# Validate desktop file syntax
desktop-file-validate ~/.local/share/applications/blackline-file-roller.desktop

# Ensure it can be found by the system
find ~/.local/share/applications/ -name "blackline-file-roller.desktop"
```

## Advanced Configuration

### Custom MIME Type Associations

Edit `~/.config/mimeapps.list` to add custom associations:

```ini
[Default Applications]
image/png=blackline-file-roller.desktop
text/plain=blackline-file-roller.desktop
application/pdf=blackline-file-roller.desktop
```

### Environment Variables

You can pass environment variables to control File Roller behavior:

```bash
# Set default zoom level (example)
FILER_ZOOM=100 blackline-file-roller image.jpg

# Set verbose output
DEBUG=1 blackline-file-roller file.txt
```

### Multiple File Arguments

File Roller can open multiple files:

```bash
blackline-file-roller file1.jpg file2.txt file3.pdf
```

## Performance Tips

1. **For large image files**: File Roller loads images into memory. Very large images (>50MB) may be slow.

2. **For large text files**: Files >1MB show a warning. Recommended text editor threshold is documented in code.

3. **For video/audio**: External players are recommended for best performance.

## Integration with Applications

### From Command Line
```bash
# Open and view any supported file
blackline-file-roller "my document.pdf"
blackline-file-roller "~/Pictures/vacation.jpg"
blackline-file-roller "/path/to/archive.zip"
```

### From Scripts
```bash
#!/bin/bash
# Script to preview files before processing
for file in *.jpg; do
    blackline-file-roller "$file"
    read -p "Continue? (y/n) "
    [ "$REPLY" = "y" ] || break
done
```

### From Desktop Shortcuts
Create a `.desktop` file to launch File Roller with specific files:

```ini
[Desktop Entry]
Type=Application
Name=My Project Files
Exec=blackline-file-roller /path/to/project
Icon=folder-open
Categories=Utility;
```

## Keyboard Shortcuts

While File Roller is open, use these shortcuts:

| Action | Shortcut |
|--------|----------|
| Open File | Ctrl+O |
| Zoom In | Ctrl+Plus (for images) |
| Zoom Out | Ctrl+Minus (for images) |
| Fit Window | Ctrl+0 (for images) |
| Rotate CW | Ctrl+R (for images) |
| Rotate CCW | Ctrl+L (for images) |
| Close | Ctrl+Q or Alt+F4 |

*Note: Actual shortcuts may vary based on implementation*

## Uninstall File Roller Integration

To remove File Roller from your system:

```bash
# Remove the setup
make uninstall

# Or manually remove:
rm ~/.local/share/applications/blackline-file-roller.desktop
update-desktop-database ~/.local/share/applications/
```

## Support

For issues with File Roller integration:

1. Check that File Roller is installed: `blackline-file-roller --version`
2. Verify the desktop file: `cat ~/.local/share/applications/blackline-file-roller.desktop`
3. Check X11/Wayland compatibility
4. Report issues with detailed error messages

## See Also

- [File Roller README.md](./README.md) - Full feature documentation
- [GDesktopAppInfo Documentation](https://developer.gnome.org/gio/stable/GDesktopAppInfo.html)
- [freedesktop.org Desktop Entry Specification](https://specifications.freedesktop.org/desktop-entry-spec/)
- [MIME Type Documentation](https://www.iana.org/assignments/media-types/media-types.xhtml)
