import fs from 'node:fs';
import path from 'node:path';

const root = path.resolve(import.meta.dirname, '..', '..');
const civilizations = JSON.parse(fs.readFileSync(path.join(root, 'data/civilizations.json'), 'utf8')).civilizations;
const commanders = JSON.parse(fs.readFileSync(path.join(root, 'data/commanders.json'), 'utf8')).commanders;

const maxSkill = (commander, type) => commander.skills
  .filter((skill) => skill.type === type)
  .flatMap((skill) => skill.effects)
  .reduce((total, effect) => total + (effect.per_level * commander.skills.find((skill) => skill.effects.includes(effect)).max_level), 0);

const summaries = civilizations.map((civilization) => {
  const roster = commanders.filter((commander) => commander.nation === civilization.id);
  return {
    id: civilization.id,
    name_ar: civilization.name_ar,
    special_unit: civilization.special_unit.id,
    starter_commander: civilization.starter_commander,
    bonus_total_percent: Math.round(civilization.bonuses.reduce((total, bonus) => total + bonus.value, 0) * 100),
    commander_count: roster.length,
    commanders: roster.map((commander) => ({
      id: commander.id,
      name: commander.name,
      rarity: commander.rarity,
      tags: commander.tags,
      base_total: Object.values(commander.base_stats).reduce((sum, value) => sum + value, 0),
      max_attack_bonus_percent: Math.round(maxSkill(commander, 'attack') * 100),
      max_defense_bonus_percent: Math.round(maxSkill(commander, 'defense') * 100),
      max_passive_bonus_percent: Math.round(maxSkill(commander, 'passive') * 100),
    })),
  };
});

const output = {
  generated_from: ['data/civilizations.json', 'data/commanders.json'],
  civilization_count: civilizations.length,
  commander_count: commanders.length,
  summaries,
};

const outputPath = path.join(root, 'design', '05-production', 'roster_summary.json');
fs.writeFileSync(outputPath, `${JSON.stringify(output, null, 2)}\n`);
console.log(`Wrote ${outputPath}`);
console.log(`Civilizations: ${output.civilization_count}; commanders: ${output.commander_count}`);
