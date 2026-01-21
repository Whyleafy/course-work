-- =========================
-- Test data (stable, no hardcoded IDs)
-- =========================
PRAGMA foreign_keys = ON;

-- --- Products ---
INSERT OR IGNORE INTO products (name, unit, price, sort, is_active) VALUES
('Мука пшеничная высший сорт', 'кг', 45.50, 1, 1),
('Мука ржаная обойная',        'кг', 38.00, 2, 1),
('Мука овсяная',               'кг', 52.00, 3, 1),
('Мука кукурузная',            'кг', 48.50, 4, 1),
('Отруби пшеничные',           'кг', 15.00, 5, 1),
('Крупа гречневая ядрица',     'кг', 85.00, 6, 1),
('Крупа рисовая',              'кг', 65.00, 7, 1),
('Крупа овсяная',              'кг', 42.00, 8, 1);

-- --- Counterparties ---
INSERT OR IGNORE INTO counterparties (name, type, address, is_active) VALUES
('ООО "Хлебзавод №1"',             'customer', 'г. Москва, ул. Хлебная, д. 10', 1),
('ИП Иванов Иван Иванович',        'supplier', 'г. Подольск, ул. Промышленная, д. 5', 1),
('ООО "Торговый дом "Мука""',      'both',     'г. Санкт-Петербург, пр. Невский, д. 100', 1),
('ЗАО "Агро-Трейд"',               'supplier', 'г. Ростов-на-Дону, ул. Сельская, д. 20', 1),
('ООО "Пекарня "Свежий хлеб""',    'customer', 'г. Казань, ул. Пекарская, д. 15', 1);

-- --- Requisites ---
-- Важно: counterparty_id может быть НЕ 1..5, поэтому тоже делаем через SELECT по имени
INSERT OR IGNORE INTO requisites (counterparty_id, inn, kpp, bik, bank_name, r_account, k_account)
SELECT id, '7701234567', '770101001', '044525225', 'ПАО "Сбербанк"', '40702810100000001234', '30101810400000000225'
FROM counterparties WHERE name='ООО "Хлебзавод №1"';

INSERT OR IGNORE INTO requisites (counterparty_id, inn, kpp, bik, bank_name, r_account, k_account)
SELECT id, '501234567890', '501201001', '044525225', 'ПАО "Сбербанк"', '40802810100000005678', '30101810400000000225'
FROM counterparties WHERE name='ИП Иванов Иван Иванович';

INSERT OR IGNORE INTO requisites (counterparty_id, inn, kpp, bik, bank_name, r_account, k_account)
SELECT id, '7812345678', '781101001', '044030795', 'ПАО "Банк ВТБ"', '40702810100000009012', '30101810400000000795'
FROM counterparties WHERE name='ООО "Торговый дом "Мука""';

INSERT OR IGNORE INTO requisites (counterparty_id, inn, kpp, bik, bank_name, r_account, k_account)
SELECT id, '6164567890', '616401001', '046015207', 'ПАО "Альфа-Банк"', '40702810100000004567', '30101810600000000207'
FROM counterparties WHERE name='ЗАО "Агро-Трейд"';

INSERT OR IGNORE INTO requisites (counterparty_id, inn, kpp, bik, bank_name, r_account, k_account)
SELECT id, '1650123456', '165001001', '049205774', 'ПАО "Тинькофф Банк"', '40702810100000007890', '30101810145250000774'
FROM counterparties WHERE name='ООО "Пекарня "Свежий хлеб""';

-- --- Documents (TTN) ---
-- sender/receiver тоже через SELECT
INSERT OR IGNORE INTO documents (doc_type, number, date, status, sender_id, receiver_id, total_amount, notes)
SELECT
  'transfer',
  'ТТН-001',
  date('now', '-2 days'),
  'POSTED',
  (SELECT id FROM counterparties WHERE name='ИП Иванов Иван Иванович'),
  (SELECT id FROM counterparties WHERE name='ООО "Хлебзавод №1"'),
  0.0,
  'ТТН на поставку муки';

INSERT OR IGNORE INTO documents (doc_type, number, date, status, sender_id, receiver_id, total_amount, notes)
SELECT
  'transfer',
  'ТТН-002',
  date('now', '-1 days'),
  'DRAFT',
  (SELECT id FROM counterparties WHERE name='ООО "Торговый дом "Мука""'),
  (SELECT id FROM counterparties WHERE name='ООО "Хлебзавод №1"'),
  0.0,
  'ТТН на поставку круп';

-- --- Document lines ---
-- Для POSTED (ТТН-001): 1000 кг муки
INSERT OR IGNORE INTO document_lines (document_id, product_id, qty_kg, price, line_sum)
SELECT
  d.id,
  p.id,
  1000.0,
  p.price,
  1000.0 * p.price
FROM documents d
JOIN products p ON p.name='Мука пшеничная высший сорт'
WHERE d.doc_type='transfer' AND d.number='ТТН-001';

-- Для DRAFT (ТТН-002): гречка 500, рис 300
INSERT OR IGNORE INTO document_lines (document_id, product_id, qty_kg, price, line_sum)
SELECT d.id, p.id, 500.0, p.price, 500.0 * p.price
FROM documents d
JOIN products p ON p.name='Крупа гречневая ядрица'
WHERE d.doc_type='transfer' AND d.number='ТТН-002';

INSERT OR IGNORE INTO document_lines (document_id, product_id, qty_kg, price, line_sum)
SELECT d.id, p.id, 300.0, p.price, 300.0 * p.price
FROM documents d
JOIN products p ON p.name='Крупа рисовая'
WHERE d.doc_type='transfer' AND d.number='ТТН-002';

-- --- Totals ---
UPDATE documents
SET total_amount = (
  SELECT COALESCE(SUM(line_sum),0) FROM document_lines WHERE document_id = documents.id
),
updated_at = datetime('now')
WHERE doc_type='transfer' AND number IN ('ТТН-001','ТТН-002');

-- --- Inventory movements for POSTED docs only (ТТН-001) ---
-- ВАЖНО: в твоей логике transfer = расход (qty_delta отрицательный)
INSERT OR IGNORE INTO inventory_movements (document_id, product_id, qty_delta_kg, movement_date, cancelled_flag)
SELECT
  d.id,
  p.id,
  -1000.0,
  d.date,
  0
FROM documents d
JOIN products p ON p.name='Мука пшеничная высший сорт'
WHERE d.doc_type='transfer' AND d.number='ТТН-001';
