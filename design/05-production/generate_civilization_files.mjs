import { mkdir, readFile, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '../..');
const output = path.join(root, 'design/02-civilizations/profiles');
const civilizations = JSON.parse(await readFile(path.join(root, 'data/civilizations.json'), 'utf8'));
const commanderData = JSON.parse(await readFile(path.join(root, 'data/commanders.json'), 'utf8'));
const commandersByNation = new Map();
for (const commander of commanderData.commanders) {
  const list = commandersByNation.get(commander.nation) ?? [];
  list.push(commander);
  commandersByNation.set(commander.nation, list);
}

const artDirection = {
  rome: {
    motif: 'قوس حجري + نسر + حافة حمراء مطفأة',
    city: 'قناطر، رواق حجري، أسقف قرميدية، وساحات منظمة بخطوط مستقيمة.',
    map: 'طرق حجرية مقروءة، أعلام مستطيلة، ونقاط حراسة لا تحجب مسار اللاعب.',
    ui: 'لوحات عاجية مع ختم نسر، خطوط فاصلة عريضة، وحالة دفاع تُقرأ بدرع مستطيل.',
    loop: 'يثبت الصف ثم يحول طريقاً آمناً إلى موضع دفاعي طويل الأجل.',
    caution: 'لا تمنح حزمة الدفاع سرعة أو ضرراً خفياً؛ تفوقها هو الاستمرارية والتنظيم فقط.'
  },
  china: {
    motif: 'سقف متدرج + فانوس + حافة ذهبية هادئة',
    city: 'أروقة خشبية مكسوة، أسقف منحنية، فوانيس محددة، وحدائق صغيرة لا تزاحم أهداف اللمس.',
    map: 'محطات إشارة وطرق لوجستية، مع ألوان دافئة واضحة فوق خضرة الخريطة.',
    ui: 'ورق مُعتّق وفواصل بختم مربع؛ حالات البناء والبحث تستعمل مؤقتاً وشريط تقدم لا لوناً فقط.',
    loop: 'يحول أفضلية الوقت إلى اقتصاد وبحث ثم يختار نقطة اشتباك مدروسة.',
    caution: 'سرعة البناء والبحث لا تختصر متطلبات المستوى ولا تكسر اقتصاد المواد.'
  },
  arabia: {
    motif: 'قبة فيروزية + قوس مدبب + ذهب رملي',
    city: 'أفنية ماء، ظلال قماش، أقواس مهوّاة، وخطوط أفقية خفيفة تحافظ على وضوح النص.',
    map: 'واحات ومحطات قافلة ومنارات كشف؛ تضاريس الرمل لا تخفض تباين نطاق التحالف.',
    ui: 'خلفية كحلية داكنة مع معدن ذهبي، وسهم مسيرة بارز يشرح السرعة وخط الرجوع.',
    loop: 'يكشف مبكراً، يختار مساراً مفتوحاً، وينقل قوة الفرسان قبل أن يكتمل تطويق الخصم.',
    caution: 'سرعة المسيرة والرؤية ليستا حصانة؛ يظل الدخول إلى نطاق عدو بلا كشف قراراً خطراً.'
  },
  egypt: {
    motif: 'مسلة + حجر كلسي + فيروز وذهب',
    city: 'محاور نهرية، أعمدة عريضة، مسلات كنقاط إرشاد، وكتل حجرية كبيرة لا تصنع ممرات ضيقة.',
    map: 'ضفاف خصبة وحجر منحوت ومواقع تخزين؛ الخطوط المائية تساعد قراءة الطرق لا تخفيها.',
    ui: 'ألواح حجر فاتح مع رموز نيلية؛ الإنتاج والعلاج يظهران بموارد ووقت متبقٍ واضحين.',
    loop: 'يبني مخزوناً آمناً، يعيد الجرحى إلى الميدان، ويستثمر الحجر في دفاع محسوب.',
    caution: 'الإنتاج والعلاج لا يتحولان إلى نقاط ضرر مباشرة في معادلة القتال.'
  },
  vikings: {
    motif: 'خشب منحوت + حديد بارد + أزرق رمادي',
    city: 'قاعات خشب طويلة، ألواح منحنية، حبال ومخازن ظاهرة؛ لا تستخدم رموز نهب على واجهات آمنة.',
    map: 'شواطئ وطرق وعرة، معسكرات برابرة ونقاط قافلة؛ تحديد الهدف الأحمر لا يخلط مع راية الحليف.',
    ui: 'خشب داكن ونقوش بسيطة، حمولة المسيرة تقرأ بأيقونة وصندوق وشريط لا برقم مجرد.',
    loop: 'يصطاد محتوى العالم ويحمل موارد أكثر ثم يحوّل الرحلات إلى زخم اقتصادي.',
    caution: 'ضرر البرابرة والحمولة لا يؤثران في قتال لاعب ضد لاعب خارج القواعد العامة.'
  },
  japan: {
    motif: 'خشب داكن + سقف حاد + قرمزي منضبط',
    city: 'بوابات توري، أسقف طبقية، حجر وحدائق مقصوصة؛ الفراغ جزء من القراءة وليس مساحة للزخرفة.',
    map: 'مسارات حجرية ونقاط رماة على المرتفعات، مع بتلات موسمية منخفضة الكثافة حتى لا تحجب الأهداف.',
    ui: 'حبر فحمي مع قرمزي للحسم فقط؛ بطاقة القائد تبرز التقدم والمهارة التالية بترتيب عمودي هادئ.',
    loop: 'يستثمر في قائد دقيق، يبني قوة رماة، ثم يحسم اشتباكاً قصيراً مع اختيار موقع جيد.',
    caution: 'زيادة الهجوم والتدريب لا تتجاوز سقوف الضرر ولا تلغي دفاع الحامية أو التضاريس.'
  }
};

const pct = (value) => `${Math.round(value * 100)}%`;
const statLabel = {
  infantry_defense: 'دفاع المشاة', troop_health: 'صحة القوات', gathering_speed: 'سرعة الجمع',
  building_speed: 'سرعة البناء', research_speed: 'سرعة البحث', action_point_recovery: 'استعادة نقاط الحركة',
  march_speed: 'سرعة المسيرة', cavalry_attack: 'هجوم الفرسان', scout_vision: 'رؤية الكشافة',
  resource_production: 'إنتاج الموارد', stone_gathering: 'جمع الحجر', healing_speed: 'سرعة العلاج',
  troop_load: 'حمولة القوات', damage_to_barbarians: 'ضرر ضد البرابرة', troop_attack: 'هجوم القوات',
  commander_xp: 'خبرة القادة', training_speed: 'سرعة التدريب', archer_attack: 'هجوم الرماة',
  troop_defense: 'دفاع القوات', infantry_attack: 'هجوم المشاة', xp_gain: 'كسب الخبرة'
};

function commanderRow(commander) {
  const total = commander.base_stats.attack + commander.base_stats.defense + commander.base_stats.utility;
  const skills = commander.skills.map((skill) => {
    const effect = skill.effects.map((entry) => `${statLabel[entry.stat] ?? entry.stat} ${pct(entry.per_level)} لكل مستوى`).join('، ');
    return `${skill.name} (${skill.type}): ${effect}`;
  }).join('<br>');
  return `| ${commander.name} | ${commander.rarity} | ${commander.tags.join('، ')} | ${commander.base_stats.attack}/${commander.base_stats.defense}/${commander.base_stats.utility} (${total}) | ${skills} |`;
}

function documentFor(civ) {
  const art = artDirection[civ.id];
  const commanders = commandersByNation.get(civ.id) ?? [];
  const bonusRows = civ.bonuses.map((bonus) => `| ${statLabel[bonus.stat] ?? bonus.stat} | ${pct(bonus.value)} |`).join('\n');
  const commanderRows = commanders.map(commanderRow).join('\n');
  const special = `${civ.special_unit.id} — ${civ.special_unit.branch} — يفتح قرب المستوى T${civ.special_unit.unlock_tier}`;
  return `# حضارة ${civ.name_ar} — ملف تصميم مستقل\n\n> **حالة البيانات:** يولّد هذا الملف من \`data/civilizations.json\` و\`data/commanders.json\`. لا تعدّل الأرقام أو المعرّفات هنا؛ عدّل المصدر ثم شغّل \`node design/05-production/generate_civilization_files.mjs\`.\n\n## الهوية واللعب\n\n| البند | التعريف |\n|---|---|\n| الفانتازيا | ${civ.fantasy_ar} |\n| الوحدة الخاصة | ${special} |\n| القائد الابتدائي | ${civ.starter_commander} |\n| لوحة الألوان المصدرية | رئيسي \`${civ.theme.primary}\`، ثانوي \`${civ.theme.secondary}\` |\n| النمط المعماري في البيانات | \`${civ.theme.architecture}\` |\n| دورة اللعب | ${art.loop} |\n\n### السرد الافتتاحي\n\n> ${civ.story.join(' ')}\n\n**التحية داخل اللعبة:** «${civ.greeting}»\n\n## المزايا التنفيذية\n\n| الإحصاء | القيمة |\n|---|---:|\n${bonusRows}\n\nهذه مكافآت هوية خارج معادلة المهارة الفردية للقائد. يعرض العميل كل مكافأة باسمها ونسبتها وشرطها؛ لا يحولها إلى «قوة» سوداء الصندوق.\n\n## القادة الحاليون\n\n| القائد | الندرة | الوسوم | إحصاءات البداية هجوم/دفاع/منفعة (المجموع) | المهارات عند كل مستوى |\n|---|---|---|---:|---|\n${commanderRows}\n\n## اتجاه الفن والواجهة\n\n| الطبقة | قرار التصميم |\n|---|---|\n| الرمز البصري | ${art.motif} |\n| المدينة | ${art.city} |\n| الخريطة | ${art.map} |\n| واجهة المستخدم | ${art.ui} |\n| الانضباط التنافسي | ${art.caution} |\n\nلا تعني الهوية البصرية وحدات أو مباني ميكانيكية إضافية. أي مبنى أو قائد أو نطاق جديد يبدأ في وثيقة نظام مستقلة ثم يدخل البيانات بعد الموازنة.\n\n## حالات واجهة الحضارة\n\n| الحالة | المعالجة |\n|---|---|\n| مختارة | حلقة تباين مزدوجة، رمز الحضارة، واسم مكتوب؛ لا لون مفرد. |\n| مقفلة | قفل + المتطلب النصي + صورة باهتة؛ لا زر خادع. |\n| مكافأة سارية | شارة إحصاء، نسبة، وسياق الأثر. |\n| قائد متاح | الندرة، الوسوم، إحصاءات البداية، والمهارة التالية في بطاقة واحدة. |\n| حدث موسمي | راية صغيرة محايدة لا تخفي حالة الملكية أو الخطر. |\n\n## أسئلة التوازن قبل التعديل\n\n| السؤال | سبب المراجعة |\n|---|---|\n| هل تعمل الميزة في طورها المقصود فقط؟ | يمنع تحويل bonus جمع أو شفاء إلى تفوق PvP غير مقصود. |\n| هل تظهر في معادلة القوة والتقرير؟ | يمنع ضرراً/دفاعاً خفياً. |\n| هل يوجد counterplay واضح؟ | يضمن أن السرعة أو الحامية أو الرؤية ليست حصانة. |\n| هل القائد والوحدة الخاصة يخدمان نفس الحلقة دون تكرار؟ | يحافظ على هوية الحضارة من دون مضاعفة bonus واحد. |\n\n## روابط مرجعية\n\n| المرجع | الاستخدام |\n|---|---|\n| [مرجع الحضارات والقادة](../CIVILIZATIONS_AND_COMMANDERS.md) | المقارنة الجامعة والحالة التنفيذية |\n| [نموذج القوة](../../03-systems/POWER_MODEL.md) | موضع المكافآت والقائد في القوة |\n| [الهوية البصرية](../../01-visual/VISUAL_IDENTITY.md) | الألوان والمكونات والوصولية |\n| [كتالوج المباني](../../04-world-map/BUILDING_CATALOG.md) | مباني المدينة والتحالف وهوية المعمار |\n`;
}

await mkdir(output, { recursive: true });
for (const civ of civilizations.civilizations) {
  await writeFile(path.join(output, `${civ.id}.md`), documentFor(civ), 'utf8');
}

const index = `# ملفات الحضارات\n\n> **مصدر الأرقام الوحيد:** \`data/civilizations.json\` و\`data/commanders.json\`. يولّد هذا المجلد من المصدرين عبر \`node design/05-production/generate_civilization_files.mjs\`.\n\n| الحضارة | الملف | الوحدة الخاصة | القائد الابتدائي |\n|---|---|---|---|\n${civilizations.civilizations.map((civ) => `| ${civ.name_ar} | [${civ.id}](${civ.id}.md) | ${civ.special_unit.id} | ${civ.starter_commander} |`).join('\n')}\n\nتجد المقارنة الشاملة في [CIVILIZATIONS_AND_COMMANDERS.md](../CIVILIZATIONS_AND_COMMANDERS.md).\n`;
await writeFile(path.join(output, 'README.md'), index, 'utf8');
console.log(`Generated ${civilizations.civilizations.length} civilization profiles in ${path.relative(root, output)}.`);
