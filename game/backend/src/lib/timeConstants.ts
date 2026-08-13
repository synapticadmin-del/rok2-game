// P10 + OPS: ثوابت زمنية مركزية — تُقرأ من هنا في كل كود المملكة.
// لا تُدخل أرقام الليترال الزمنية مباشرة في KingdomShard أو handlers
// (بروتوكول anti-hardcoding: verify_p7_t15_ops يرفض أي ظهور لـ 3600000/86400000 في الشارد).
const MS_PER_HOUR = 86400000 / 24;
const MS_PER_DAY = 86400000;
const MS_PER_MINUTE = 60000;
export { MS_PER_HOUR, MS_PER_DAY, MS_PER_MINUTE };
