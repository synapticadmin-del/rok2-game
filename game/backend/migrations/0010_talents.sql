-- ROK2 D1 talents schema (P8-T1): نظام مواهب القادة — شجرتا troop_type وrole
ALTER TABLE player_commanders ADD COLUMN talents_json TEXT NOT NULL DEFAULT '{}';
ALTER TABLE march_commanders ADD COLUMN talents_json TEXT NOT NULL DEFAULT '{}';
