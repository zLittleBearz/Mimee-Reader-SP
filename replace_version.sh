#!/usr/bin/env bash
set -euo pipefail

OLD="Mimee Reader SP 1.5.2"
NEW="Mimee Reader SP 1.5.2"

FILES=$(grep -rIl --exclude-dir=.git -F "$OLD" . || true)

if [ -z "$FILES" ]; then
  echo "ไม่พบสตริง '$OLD' ในโปรเจกต์"
  exit 0
fi

echo "พบในไฟล์ต่อไปนี้:"
echo "$FILES"
echo "---"

echo "$FILES" | while IFS= read -r f; do
  perl -pi -e "BEGIN{\$old=quotemeta('$OLD'); \$new='$NEW';} s/\$old/\$new/g" "$f"
  echo "แก้ไขแล้ว: $f"
done

echo "---"
echo "เสร็จแล้ว ตรวจสอบด้วย: git diff"
