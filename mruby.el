;;; mruby.el --- Emacs with mruby

;; Copyright (C) 2015 by Syohei YOSHIDA

;; Author: Syohei YOSHIDA <syohex@gmail.com>
;; URL: https://github.com/syohex/emacs-mruby-test
;; Version: 0.01

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;; Test package of Emacs dynamic module

;;; Code:

(require 'mruby-core)

(defvar mruby-last-value nil)

(defun mruby--eval (code)
  (let ((mrb (mruby-init)))
    (prog1 (setq mruby-last-value (mruby-eval mrb code))
      (message "%s" mruby-last-value))))

;;;###autoload
(defun mruby-eval-region (beg end)
  (interactive "r")
  (mruby--eval (buffer-substring-no-properties beg end)))

;;;###autoload
(defun mruby-eval-buffer ()
  (interactive)
  (mruby-eval-region (point-min) (point-max)))

;;;###autoload
(defun mruby-eval-file (file)
  (interactive
   (list (read-file-name "mruby file: " nil nil t)))
  (let ((code (with-temp-buffer
                (insert-file-literally file)
                (buffer-string))))
    (mruby--eval code)))

(provide 'mruby)

;;; mruby.el ends here
