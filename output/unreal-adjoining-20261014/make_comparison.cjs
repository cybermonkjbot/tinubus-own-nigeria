// Arrange actual matched Unreal captures without retouching scene content.
const path = require('node:path');
const sharp = require('sharp');

async function main() {
  const folder = path.join(__dirname, 'evidence', 'matched');
  const sites = ['unity-avenue-19', 'theatre-street-2', 'theatre-street-6'];
  const first = await sharp(path.join(folder, `${sites[0]}-street-before.png`)).metadata();
  const {width, height} = first;
  const caption = 30;
  const layers = [];
  for (const [row, site] of sites.entries()) {
    for (const [column, state] of ['before', 'after'].entries()) {
      const file = path.join(folder, `${site}-street-${state}.png`);
      const info = await sharp(file).metadata();
      if (info.width !== width || info.height !== height) throw Error('Viewport changed');
      const x = column * width, y = row * (height + caption);
      const label = `${site.replaceAll('-', ' ')} | ${state}`;
      const header = Buffer.from(`<svg width="${width}" height="${caption}"><text x="10" y="21" font-size="15" fill="white">${label}</text></svg>`);
      layers.push({input: header, left: x, top: y}, {input: file, left: x, top: y + caption});
    }
  }
  const target = path.join(__dirname, 'evidence', 'before-after.png');
  await sharp({create: {width: width * 2, height: (height + caption) * sites.length, channels: 3, background: '#191919'}})
    .composite(layers).png().toFile(target);
  console.log(target);
}
main().catch(error => { console.error(error); process.exitCode = 1; });
