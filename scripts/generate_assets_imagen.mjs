// Copyright ROK2. Automated Image Generator using Google Imagen 3 / Gemini Nano.
import fs from 'node:fs';
import path from 'node:path';

const API_KEY = process.env.GEMINI_API_KEY || process.env.GOOGLE_API_KEY;
const OUTPUT_DIR = path.resolve('game/client-unreal/Content/Art/GeneratedAssets');

if (!fs.existsSync(OUTPUT_DIR)) {
  fs.mkdirSync(OUTPUT_DIR, { recursive: true });
}

const ASSETS_QUEUE = [
  {
    name: 'city_hall_base_tier1',
    prompt: 'Isometric 3D game asset of a Level 1-5 City Hall chieftain keep, common early medieval settlement, sturdy oak timber log walls, layered thatched straw and wooden shingle roof, reinforced double entrance door with iron fittings, small wooden watchtower balcony with a blank canvas banner, raised river-stone foundation, stylized hand-painted 3D art style, mobile 4X strategy game visual, clean crisp silhouette, 45 degree orthographic isometric camera angle, warm sunlight from top-left, soft ambient blue fill light, isolated on solid white background, high quality concept render.',
  },
  {
    name: 'barracks_base_tier1',
    prompt: 'Isometric 3D game asset of a Level 1-5 Infantry Barracks, early medieval military training camp, rough-hewn timber palisade walls, canvas training tent with timber roof, crossed iron swords above the wooden archway, wooden combat sparring dummies with straw padding, weapon rack with spears and shields, stylized hand-painted 3D art style, mobile 4X strategy aesthetic, clean silhouette, 45 degree isometric angle, warm directional lighting, isolated on solid white background.',
  },
  {
    name: 'farm_base_tier1',
    prompt: 'Isometric 3D game asset of a Level 1-5 Farm, rustic medieval wooden windmill with canvas sails, attached small grain silo with wooden shingles, golden wheat sheaf bundles stacked by a split-rail wooden fence, small vegetable patch, stylized hand-painted 3D art style, mobile strategy game aesthetic, 45 degree isometric perspective, bright warm sunlight, isolated on solid white background.',
  },
  {
    name: 'hospital_base_tier1',
    prompt: 'Isometric 3D game asset of a Level 1-5 Field Hospital, medieval wooden medical lodge with clean white canvas awning canopy, vibrant red medical cross emblem on the cloth awning, stone mortar with healing herbs, wooden stretchers and herbal drying racks, stylized hand-painted 3D art style, mobile 4X strategy aesthetic, 45 degree isometric view, warm soothing lighting, isolated on solid white background.',
  },
  {
    name: 'wall_base_tier1',
    prompt: 'Isometric 3D game asset of a Level 1-5 Castle Wall & Gate, fortified wooden log palisade with twin square timber watchtowers, heavy iron-studded wooden portcullis gate, raised stone base, flaming torches on timber posts, stylized hand-painted 3D art style, 45 degree isometric perspective, dramatic sunlight, isolated on solid white background.',
  },
  {
    name: 'civ_rome_hall_tier4',
    prompt: 'Isometric 3D game asset of a Level 18 Roman Imperial City Hall, grand Capitoline palace temple, polished white Carrara marble columns with Corinthian capitals, terracotta red tile roof, grand bronze dome with gilded finials, golden Aquila Roman eagle standard above the monumental marble portico, crimson silk banners with gold laurels, marble mosaic steps, stylized hand-painted 3D art style, mobile 4X strategy aesthetic, 45 degree orthographic isometric view, dramatic warm Mediterranean sunlight, isolated on solid white background.',
  },
  {
    name: 'civ_china_hall_tier4',
    prompt: 'Isometric 3D game asset of a Level 18 Chinese Imperial City Hall, Forbidden Palace grand palace hall, triple-tiered sweeping glazed golden yellow tile roofs with Dou-gong brackets, crimson lacquer pillars, carved marble balustrades, pair of golden stone Foo Dog guardian lions at entrance, red silk palace lanterns, stylized hand-painted 3D art style, mobile 4X strategy aesthetic, 45 degree isometric view, majestic imperial lighting, isolated on solid white background.',
  },
  {
    name: 'civ_arabia_hall_tier4',
    prompt: 'Isometric 3D game asset of a Level 18 Arabian Royal Citadel City Hall, majestic palace of 1001 nights, brilliant turquoise tiled central dome with golden filigree crescent spire, four graceful perimeter minarets, elegant horseshoe arches with intricate geometric arabesque carvings, cream sun-baked sandstone walls, central fountain courtyard with blue zellige tiles, stylized hand-painted 3D art style, 45 degree isometric angle, warm desert golden hour lighting, isolated on solid white background.',
  },
  {
    name: 'civ_egypt_hall_tier4',
    prompt: 'Isometric 3D game asset of a Level 18 Egyptian Pharaoh Temple City Hall, monumental Karnak temple complex, massive sloping stone pylon gates with gold-leaf cavetto cornices, twin towering gilded electrum-tipped obelisks at entrance, columns shaped like blooming papyrus bundles, yellow desert sandstone with carved hieroglyphic reliefs and winged sun disc, stylized hand-painted 3D art style, 45 degree isometric perspective, brilliant Nile sun lighting, isolated on solid white background.',
  },
  {
    name: 'civ_vikings_hall_tier4',
    prompt: 'Isometric 3D game asset of a Level 18 Viking Jarl Great Hall, monumental Norse fortress longhouse, massive dark pine timber beams with intricately carved dragon-head prow gables, steep moss-thatched roof reinforced with iron ribbing, stone foundation carved with glowing frost runes, twin wooden watchtowers with roaring braziers, painted round shields mounted along exterior walls, stylized hand-painted 3D art style, 45 degree isometric view, dramatic Nordic lighting, isolated on solid white background.',
  },
  {
    name: 'civ_japan_hall_tier4',
    prompt: 'Isometric 3D game asset of a Level 18 Japanese Tenshu Castle City Hall, imposing feudal fortress keep, multi-tiered dark charcoal wood roofs with curving eaves, golden Shachihoko mythical carp finials on roof ridges, stark white plaster walls on massive sloping stone base (musha-gaeshi), vermilion red lacquered balcony trims, crimson Torii gate portal, stylized hand-painted 3D art style, 45 degree isometric angle, isolated on solid white background.',
  }
];

async function generateWithImagen(item) {
  const url = `https://generativelanguage.googleapis.com/v1beta/models/imagen-3.0-generate-002:predict?key=${API_KEY}`;
  const payload = {
    instances: [{ prompt: item.prompt }],
    parameters: {
      sampleCount: 1,
      aspectRatio: '1:1',
      outputMimeType: 'image/png'
    }
  };

  console.log(`\n[Generating] ${item.name}...`);
  const response = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });

  if (!response.ok) {
    const errText = await response.text();
    throw new Error(`API Error (${response.status}): ${errText}`);
  }

  const data = await response.json();
  const base64Data = data.predictions?.[0]?.bytesBase64Encoded;
  if (!base64Data) throw new Error('No image data returned from API.');

  const filePath = path.join(OUTPUT_DIR, `${item.name}.png`);
  fs.writeFileSync(filePath, Buffer.from(base64Data, 'base64'));
  console.log(`✓ Saved: ${filePath}`);
}

async function main() {
  if (!API_KEY) {
    console.error('\n[Error] Missing GEMINI_API_KEY or GOOGLE_API_KEY environment variable.');
    console.log('To run batch generation:');
    console.log('  $env:GEMINI_API_KEY="your_api_key_here"; node scripts/generate_assets_imagen.mjs\n');
    process.exit(1);
  }

  console.log(`Starting generation for ${ASSETS_QUEUE.length} assets...`);
  for (const item of ASSETS_QUEUE) {
    try {
      await generateWithImagen(item);
    } catch (err) {
      console.error(`✗ Failed for ${item.name}:`, err.message);
    }
  }
  console.log('\n=== Batch Generation Finished ===');
}

main();
