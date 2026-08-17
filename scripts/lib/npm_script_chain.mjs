/**
 * npm_script_chain.mjs — توسيع سلسلة سكربتات npm تعدياً.
 *
 * لماذا: عدة حرّاس تتحقق أن وظيفةً ما «داخل بوابة check» بالبحث الحرفي في
 * `pkg.scripts.check`. حين صارت البوابة مركّبة —
 *     "check": "npm run check:fast && npm run check:e2e && npm run check:ue-contracts"
 * — بقيت كل الوظائف تُشغَّل فعلاً، لكن البحث الحرفي لم يجدها فأبلغ الحرّاس
 * غياباً وهمياً. والعطل المقابل أسوأ: لو نُقلت وظيفة خارج السلسلة حقاً، فحصٌ
 * يقرأ سطراً واحداً لن يرصده إن بقي الاسم مذكوراً في سطر آخر.
 *
 * الحل: نوسّع كل `npm run X` إلى نص السكربت X تعدياً، مع حماية من الدور
 * (سكربت يشير إلى نفسه مباشرةً أو عبر وسيط) — فيبقى الفحص صادقاً مهما أُعيد
 * تنظيم السلسلة.
 */

const NPM_RUN_REFERENCE = /npm run (?:--silent |-s )?([\w:.@/-]+)/g;

/**
 * @param {Record<string, string>} scripts حقل scripts من package.json
 * @param {string} entry اسم السكربت المدخل (عادةً "check")
 * @returns {string} نص السلسلة موسّعاً بكل ما تستدعيه تعدياً
 */
export function resolveScriptChain(scripts, entry = 'check') {
  const expand = (name, seen) => {
    if (seen.has(name)) return '';
    const body = scripts?.[name];
    if (typeof body !== 'string') return '';

    // نسخة مستقلة من `seen` لكل فرع: مسارٌ يمرّ بسكربت لا يمنع فرعاً شقيقاً
    // من توسيعه، والدور وحده هو ما نقطعه.
    return body.replace(NPM_RUN_REFERENCE, (match, ref) => {
      const branch = new Set(seen);
      branch.add(name);
      return `${match} ${expand(ref, branch)}`;
    });
  };

  return expand(entry, new Set());
}

/**
 * هل تُشغَّل وظيفة ما داخل سلسلة؟ يقبل اسم الوظيفة كما يُكتب في package.json.
 *
 * @param {Record<string, string>} scripts
 * @param {string} jobName
 * @param {string} entry
 */
export function chainRuns(scripts, jobName, entry = 'check') {
  return resolveScriptChain(scripts, entry).includes(jobName);
}
