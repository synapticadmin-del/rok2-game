const fs = require('fs');
const path = require('path');

// CRC32 implementation
const crcTable = new Uint32Array(256);
for (let i = 0; i < 256; i++) {
  let c = i;
  for (let j = 0; j < 8; j++) {
    c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
  }
  crcTable[i] = c;
}

function crc32(buf) {
  let c = 0xFFFFFFFF;
  for (let i = 0; i < buf.length; i++) {
    c = crcTable[(c ^ buf[i]) & 0xFF] ^ (c >>> 8);
  }
  return (c ^ 0xFFFFFFFF) >>> 0;
}

function createStoredZip(sourceDir, outputFile) {
  const files = [];

  function walk(dir, rel) {
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const entry of entries) {
      const fullPath = path.join(dir, entry.name);
      const relPath = rel ? `${rel}/${entry.name}` : entry.name;
      if (entry.isDirectory()) {
        walk(fullPath, relPath);
      } else if (entry.isFile()) {
        files.push({ fullPath, relPath: relPath.replace(/\\/g, '/') });
      }
    }
  }

  walk(sourceDir, '');
  console.log(`Found ${files.length} files to pack into stored zip.`);

  const chunks = [];
  const centralEntries = [];
  let currentOffset = 0;

  for (const f of files) {
    const content = fs.readFileSync(f.fullPath);
    const nameBuf = Buffer.from(f.relPath, 'utf8');
    const crc = crc32(content);
    const size = content.length;

    // Local file header (30 bytes)
    const localHeader = Buffer.alloc(30);
    localHeader.writeUInt32LE(0x04034b50, 0); // signature
    localHeader.writeUInt16LE(20, 4);         // version needed: 2.0
    localHeader.writeUInt16LE(0, 6);          // flags
    localHeader.writeUInt16LE(0, 8);          // compression: 0 (STORED)
    localHeader.writeUInt16LE(0, 10);         // mod time
    localHeader.writeUInt16LE(0, 12);         // mod date
    localHeader.writeUInt32LE(crc, 14);       // CRC32
    localHeader.writeUInt32LE(size, 18);      // compressed size
    localHeader.writeUInt32LE(size, 22);      // uncompressed size
    localHeader.writeUInt16LE(nameBuf.length, 26); // name length
    localHeader.writeUInt16LE(0, 28);         // extra field length

    const headerOffset = currentOffset;
    chunks.push(localHeader, nameBuf, content);
    currentOffset += 30 + nameBuf.length + size;

    // Central directory header (46 bytes)
    const centralHeader = Buffer.alloc(46);
    centralHeader.writeUInt32LE(0x02014b50, 0); // signature
    centralHeader.writeUInt16LE(20, 4);          // version made by
    centralHeader.writeUInt16LE(20, 6);          // version needed
    centralHeader.writeUInt16LE(0, 8);           // flags
    centralHeader.writeUInt16LE(0, 10);          // compression: 0 (STORED)
    centralHeader.writeUInt16LE(0, 12);          // mod time
    centralHeader.writeUInt16LE(0, 14);          // mod date
    centralHeader.writeUInt32LE(crc, 16);        // CRC32
    centralHeader.writeUInt32LE(size, 20);       // compressed size
    centralHeader.writeUInt32LE(size, 24);       // uncompressed size
    centralHeader.writeUInt16LE(nameBuf.length, 28); // name length
    centralHeader.writeUInt16LE(0, 30);          // extra field length
    centralHeader.writeUInt16LE(0, 32);          // file comment length
    centralHeader.writeUInt16LE(0, 34);          // disk number start
    centralHeader.writeUInt16LE(0, 36);          // internal file attributes
    centralHeader.writeUInt32LE(0, 38);          // external file attributes
    centralHeader.writeUInt32LE(headerOffset, 42); // relative offset of local header

    centralEntries.push(centralHeader, nameBuf);
  }

  const centralOffset = currentOffset;
  let centralSize = 0;
  for (const c of centralEntries) {
    centralSize += c.length;
    chunks.push(c);
  }

  // End of Central Directory Record (22 bytes)
  const eocd = Buffer.alloc(22);
  eocd.writeUInt32LE(0x06054b50, 0);          // signature
  eocd.writeUInt16LE(0, 4);                   // disk number
  eocd.writeUInt16LE(0, 6);                   // disk with central dir
  eocd.writeUInt16LE(files.length, 8);         // entries on this disk
  eocd.writeUInt16LE(files.length, 10);        // total entries
  eocd.writeUInt32LE(centralSize, 12);         // central dir size
  eocd.writeUInt32LE(centralOffset, 16);       // central dir offset
  eocd.writeUInt16LE(0, 20);                  // comment length

  chunks.push(eocd);

  const outDir = path.dirname(outputFile);
  if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });
  fs.writeFileSync(outputFile, Buffer.concat(chunks));
  console.log(`Created stored ZIP: ${outputFile} (total size: ${fs.statSync(outputFile).size} bytes)`);
}

const sourceDir = path.resolve(process.argv[2] || 'Saved/StagedBuilds/Android_ASTC');
const outputFile = path.resolve(process.argv[3] || 'Intermediate/Android/arm64/gradle/app/src/main/assets/main.obb.png');

createStoredZip(sourceDir, outputFile);
