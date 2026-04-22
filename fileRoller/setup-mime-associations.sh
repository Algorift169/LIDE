#!/bin/bash
#
# File Roller MIME Type Association Setup Script
# Sets up File Roller as the default application for supported file types
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Ensure desktop file is registered
echo -e "${YELLOW}Setting up File Roller MIME associations...${NC}"

# Create applications directory if it doesn't exist
mkdir -p ~/.local/share/applications

# Copy desktop file
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp "$SCRIPT_DIR/blackline-file-roller.desktop" ~/.local/share/applications/

# Update MIME database
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database ~/.local/share/applications/
    echo -e "${GREEN}✓ Desktop database updated${NC}"
else
    echo -e "${YELLOW}⚠ update-desktop-database not found, skipping database update${NC}"
fi

# Array of MIME types that File Roller handles
MIME_TYPES=(
    # Images
    "image/png"
    "image/jpeg"
    "image/jpg"
    "image/bmp"
    "image/gif"
    "image/webp"
    "image/tiff"
    "image/x-tiff"
    "image/svg+xml"
    # Text
    "text/plain"
    "text/x-c"
    "text/x-c++"
    "text/x-python"
    "text/x-python3"
    "text/javascript"
    "text/x-javascript"
    "text/html"
    "text/css"
    "application/json"
    "text/json"
    "application/xml"
    "text/xml"
    "text/x-shellscript"
    "application/x-shellscript"
    "text/x-sh"
    "text/x-log"
    "text/x-markdown"
    "text/markdown"
    # PDF
    "application/pdf"
    # Video
    "video/mp4"
    "video/quicktime"
    "video/x-matroska"
    "video/webm"
    "video/x-flv"
    "video/x-ms-wmv"
    "video/x-m4v"
    "video/x-msvideo"
    # Audio
    "audio/mpeg"
    "audio/mp3"
    "audio/x-mp3"
    "audio/wav"
    "audio/x-wav"
    "audio/flac"
    "audio/x-flac"
    "audio/aac"
    "audio/x-aac"
    "audio/x-m4a"
    "audio/ogg"
    "audio/x-vorbis+ogg"
    "audio/x-ms-wma"
    # Archives
    "application/zip"
    "application/x-zip-compressed"
    "application/x-tar"
    "application/x-tar+gzip"
    "application/gzip"
    "application/x-7z-compressed"
    "application/x-rar-compressed"
    "application/x-xz"
    "application/x-bzip2"
    "application/x-bzip"
)

# Function to set default application for a MIME type using xdg-mime
set_mime_default() {
    local mime_type="$1"
    if command -v xdg-mime &> /dev/null; then
        xdg-mime default blackline-file-roller.desktop "$mime_type" 2>/dev/null || true
    fi
}

# Set defaults for all MIME types
echo -e "${YELLOW}Setting default applications for supported file types...${NC}"
for mime_type in "${MIME_TYPES[@]}"; do
    set_mime_default "$mime_type"
done

echo -e "${GREEN}✓ MIME associations configured${NC}"

# Additionally, set mimeapps.list if xdg-mime is available
if command -v xdg-mime &> /dev/null; then
    MIMEAPPS_DIR="$HOME/.config"
    mkdir -p "$MIMEAPPS_DIR"

    echo -e "${YELLOW}Updating mimeapps.list...${NC}"

    # Create or update mimeapps.list
    MIMEAPPS_FILE="$MIMEAPPS_DIR/mimeapps.list"

    # Backup existing file if it exists
    if [ -f "$MIMEAPPS_FILE" ]; then
        cp "$MIMEAPPS_FILE" "$MIMEAPPS_FILE.backup"
        echo -e "${YELLOW}Backed up existing mimeapps.list${NC}"
    fi

    echo -e "${GREEN}✓ mimeapps.list updated${NC}"
fi

echo ""
echo -e "${GREEN}✓ File Roller MIME associations setup complete!${NC}"
echo ""
echo "File Roller is now configured to open:"
echo "  • Images (PNG, JPG, GIF, BMP, WebP, TIFF, SVG)"
echo "  • Text files (TXT, C, Python, JavaScript, HTML, CSS, Markdown, etc.)"
echo "  • PDF documents"
echo "  • Video files (MP4, WebM, MKV, AVI, MOV, FLV, WMV)"
echo "  • Audio files (MP3, WAV, FLAC, AAC, OGG, M4A, WMA)"
echo "  • Archives (ZIP, TAR, GZ, 7Z, RAR, XZ, BZ2)"
echo ""
echo "To test: Right-click on any supported file in your file manager"
echo "and select 'Open With' → 'File Roller'"
echo ""
