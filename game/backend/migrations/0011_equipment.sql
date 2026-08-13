-- ROK2 D1 equipment schema (P8-T2): نظام معدات القادة / الحدادة
-- inventory + equipped لكل قائد تُخزن في equipment_json على player_commanders
-- وتُنسخ للمسيرات النشطة على march_commanders عند تعيين القائد.
ALTER TABLE player_commanders ADD COLUMN equipment_json TEXT NOT NULL DEFAULT '{}';
ALTER TABLE march_commanders ADD COLUMN equipment_json TEXT NOT NULL DEFAULT '{}';
