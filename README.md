# mruby.el

**This is a test package of dynamic module feature which is implemented in Emacs 25.**

mruby.el provides interface to mruby make interface to embed mruby into go.


## Setup

Dynamic module feature is implemented only in emacs-25 branch. So you need to build Emacs which enables dynamic module feature.

### Build Emacs

```
% git clone git://git.savannah.gnu.org/emacs.git
% cd emacs
% git checkout -b emacs-25 origin/emacs-25
% ./configure --with-modules --prefix=installed_path # You must set --with-modules
% make
% make install
```

### Building this plugin

```
% cd modules
% git clone --recursive https://github.com/syohex/emacs-mruby-test.git
% make
```


## Evaluate mruby code

```lisp
(let ((mrb (mruby-init)))
  (mruby-eval mrb "ARGV.map {|x| x + 1}" 1 2 3))
;; => [2 3 4]


(let ((mrb (mruby-init)))
  (mruby-eval mrb
              "
def fizzbuzz(num)
  (1..num).map do |n|
    case
    when n % 15 == 0 then \"fizzbuzz\"
    when n % 5  == 0 then \"buzz\"
    when n % 3  == 0 then \"fizz\"
    else n
    end
  end
end

fizzbuzz(ARGV[0].to_i)
" 20))

;; => [1 2 "fizz" 4 "buzz" "fizz" 7 8 "fizz" "buzz" 11 "fizz" 13 14 "fizzbuzz" 16 17 "fizz" 19 "buzz"]
```


## Call mruby function for elisp objects

``` lisp
;; Apply 'Fixnum#abs' for Emacs Lisp integer value
(let ((mrb (mruby-init)))
  (mruby-send mrb -10 'abs))
;; 10

;; Apply 'String#swapcase' for Emacs Lisp string value
(let ((mrb (mruby-init)))
  (mruby-send mrb "hello WORLD" 'swapcase))
;; => "HELLO world"

;; Apply 'Array#flatten' for Emacs Lisp vector value
;; [NOTE] You must use vector instead of list for Ruby Array feature
(let ((mrb (mruby-init)))
  (mruby-send mrb [1 [2 3] [4 [5 6]]] 'flatten))
;; => '[1 2 3 4 5 6]
```


## License

- This software is released under the GPLv3+.
- mruby is released under the [MIT License](https://github.com/mruby/mruby/blob/master/MITL).
