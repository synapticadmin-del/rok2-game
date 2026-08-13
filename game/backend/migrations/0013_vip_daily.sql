-- P9-T4: نظام VIP الكامل — نقاط VIP يومية (40/يوم + 20 اتصال، سقف 200)
-- أعمدة المتابعة على player_vip
ALTER TABLE player_vip ADD COLUMN last_daily_points_day INTEGER NOT NULL DEFAULT -1;
ALTER TABLE player_vip ADD COLUMN last_login_day INTEGER NOT NULL DEFAULT -1;
