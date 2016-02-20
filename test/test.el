;;; test.el --- mruby binding test

;; Copyright (C) 2015 by Syohei YOSHIDA

;; Author: Syohei YOSHIDA <syohex@gmail.com>

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

;;; Code:

(require 'ert)
(require 'mruby-core)

(ert-deftest boolean ()
  "boolean type"
  (let ((mrb (mruby-init)))
    (should (mruby-eval mrb "true"))
    (should-not (mruby-eval mrb "false"))))

(ert-deftest fixnum ()
  "fixnum type"
  (let ((mrb (mruby-init)))
    (should (= (mruby-eval mrb "1") 1))
    (should (= (mruby-eval mrb "1 + 1") 2))
    (should (= (mruby-eval mrb "(1..10).inject {|s,i| s + i}") 55))))

(ert-deftest float ()
  "float type"
  (let ((mrb (mruby-init)))
    (should (= (mruby-eval mrb "0.5") 0.5))
    (should (= (mruby-eval mrb "0.25") 0.25))))

(ert-deftest string ()
  "string type"
  (let ((mrb (mruby-init)))
    (should (string= (mruby-eval mrb "\"hello world\"") "hello world"))
    (should (string= (mruby-eval mrb "\"hello\" + \"world\"") "helloworld"))))

(ert-deftest array ()
  "array type"
  (let ((mrb (mruby-init)))
    (should-not (mruby-eval mrb "[]"))
    (should (equal (mruby-eval mrb "[1,2,3]") '[1 2 3]))
    (should (equal (mruby-eval mrb "[1,[2,3],[4,[5]]]") '[1 [2 3] [4 [5]]]))))

(ert-deftest hash ()
  "hash type"
  (let ((mrb (mruby-init)))
    (let ((hash (mruby-eval mrb "{\"name\" => [\"john\", \"smith\"], \"age\" => 39}")))
      (should (equal (gethash "name" hash) '["john" "smith"]))
      (should (= (gethash "age" hash) 39)))))

(ert-deftest argv ()
  "With ARGV type"
  (let ((mrb (mruby-init)))
    (let ((got (mruby-eval mrb "ARGV.map{|x| x + 1}" 1 2 3)))
      (should (equal got '[2 3 4])))))

(ert-deftest send-with-elisp-object ()
  "Send message with elisp object"
  (let ((mrb (mruby-init)))
    (let ((mrb (mruby-init)))
      (should (= (mruby-send mrb -10 'abs) 10)))

    (let ((got (mruby-send mrb [1 [2 3] [4 [5 6]]] 'flatten)))
      (should (equal got '[1 2 3 4 5 6])))

    (let ((got (mruby-send mrb "hello WORLD" 'swapcase)))
      (should (equal got "HELLO world")))))

(ert-deftest def-class ()
  "define class"
  (let ((mrb (mruby-init)))
    (let ((got (mruby-eval mrb "
class Editor
  def initialize(name, version)
    @name = name
    @version = version
  end

  def to_s
    \"[\" + @name + \", \" + @version.to_s + \"]\"
  end
end
Editor.new(\"Vim\", 7).to_s
")))
      (should (string= got "[Vim, 7]")))))

;;; test.el ends here
