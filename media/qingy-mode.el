;; qingy-mode.el --- Major modes for Qingy setup files

;; Copyright (C) 2003 Paolo Gianrossi 

;; Emacs Lisp Archive Entry
;; Filename: qingy-mode.el
;; Author: Paolo Gianrossi <paolino.gnu@disi.unige.it>
;; Maintainer: Paolo Gianrossi <paolino.gnu@disi.unige.it>
;; Edited by: Michele Noberasco <noberasco.gnu@disi.unige.it>
;; Version: 1.1
;; Created: 01/16/03
;; Revised: 14/01/04
;; Description: Major modes for Qingy setup files
;; URL: http://yersinia.org/homes/paolino/emacs/

;;  This file is not part of GNU Emacs.

;; This program is free software;  you can redistribute it and/or 
;; modify it under the terms of the GNU General Public License as  
;; published by the Free Software Foundation; either version 2 of the 
;; License, or (at your option) any later version.

;; This program is distributed in the hope that it will be useful, but 
;; WITHOUT ANY WARRANTY; without even the implied warranty of 
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE.  See the  GNU 
;; General Public License for more details.

;; You  should  have received  a copy of the GNU General Public 
;; License  along with this program; if not, write to the Free 
;; Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
;; 02111-1307, USA.


(defvar qingy-mode-hook nil)

(defvar qingy-mode-map
  (let ((qingy-mode-map (make-keymap)))
    (define-key qingy-mode-map "\C-j" 'newline-and-indent)
    qingy-mode-map)
  "Keymap for Qingy major mode")

(add-to-list 'auto-mode-alist '("/\\(?:settings\\|theme\\)$" . qingy-mode))

(defconst qingy-font-lock-keywords-1
  (list
	 '("\\(l\\(?:ast_session_policy\\|ast_user_policy\\)\\)" . font-lock-variable-name-face)
   '("\\(lastsession\\)" . font-lock-constant-face)
   '("\\(t\\(?:hemes_dir\\|ext_sessions\\|emp_files_dir\\|ext_mode\\)\\)" . font-lock-variable-name-face)
	 '("\\(x\\(?:_sessions\\|_args\\|init\\|_server\\)\\)" . font-lock-variable-name-face)
   '("\\(c\\(?:lear_background\\)\\|l\\(?:ock_sessions\\)\\|s\\(?:creensavers_dir\\|creensaver\\|hutdown_policy\\|ession\\)\\)" . font-lock-variable-name-face)
	 '("\\(k\\(?:ill\\)\\)" . font-lock-variable-name-face)
	 '("\\(n\\(?:ative_resolution\\|ext_tty\\)\\)" . font-lock-variable-name-face)
	 '("\\(p\\(?:assword\\|rev_tty\\|oweroff\\|re_gui_script\\|ost_gui_script\\)\\)" . font-lock-variable-name-face)
	 '("\\(q\\(?:ingy_DirectFB\\)\\)" . font-lock-variable-name-face)
	 '("\\(r\\(?:elogin\\|eboot\\)\\)" . font-lock-variable-name-face)
	 '("\\(s\\(?:leep\\)\\)" . font-lock-variable-name-face)
	 '("\\(u\\(?:sername\\)\\)" . font-lock-variable-name-face)
	 '("\\(u\\(?:ser\\)\\)" . font-lock-constant-face)
   '("\\(content\\|default_\\(?:\\(?:cursor\\|text\\)_color\\)\\|linkto\\|text_orientation\\)" . font-lock-variable-name-face)
   '("\\(b\\(?:ackground\\|utton_opacity\\)\\|cursor_color\\|font\\|text_color\\|other_text_color\\|\\(?:selected_\\)?window_opacity\\)" . font-lock-variable-name-face)
   '("\\(center\\|\\(?:lef\\|righ\\)t\\)" . font-lock-constant-face)
   '("\\(yes\\|no\\|NULL\\|global\\)" . font-lock-constant-face)
   '("\\(theme\\|window\\|tty\\|autologin\\|keybindings\\)" . font-lock-builtin-face)
   '("\\(everyone\\|noone\\|p\\(?:hoto\\|ixel\\)\\|r\\(?:andom\\|oot\\)\\)" . font-lock-constant-face)
   '("\\(large\\|medium\\|smaller\\|small\\|tiny\\)" . font-lock-constant-face)
   '("\\(\\[[ \t]*[0-9a-fA-F]\\{8\\}[ \t]*\\]\\)\\|\\([0-9]+\\)" . font-lock-constant-face)
   '("\\(c\\(?:ommand\\|ursor_color\\)\\|height\\|t\\(?:ext_\\(?:color\\|size\\)\\|\\(?:im\\|yp\\)e\\)\\|width\\|[xy]\\)" . font-lock-variable-name-face)
   )
  "Balls-out highlighting in Qingy mode")

(defvar qingy-font-lock-keywords qingy-font-lock-keywords-1
  "Default highlighting expressions for WPDL mode")

(defun qingy-indent-line ()
  "Indent current line as Qingy settings syntax"
  (interactive)
  (beginning-of-line)
  
(if (bobp)  ; Check for rule 1
      (indent-line-to 0)

  (let ((not-indented t) cur-indent)
    (if (looking-at "^[ \t]*}") ; Check for rule 2
            (progn
              (save-excursion
                (forward-line -1)
                (setq cur-indent (- (current-indentation) default-tab-width)))

	      (if (< cur-indent 0)
                  (setq cur-indent 0)))

      (save-excursion 
	(while not-indented
	  (forward-line -1)
	  (if (looking-at "^[ \t]*}") ; Check for rule 3
	      (progn
		(setq cur-indent (current-indentation))
		(setq not-indented nil))
					; Check for rule 4
	    (if (looking-at "^[ \t]*{")
		(progn
		  (setq cur-indent (+ (current-indentation) default-tab-width))
		  (setq not-indented nil))
	      (if (bobp) ; Check for rule 5
		  (setq not-indented nil)))))))
    (if cur-indent
	(indent-line-to cur-indent)
      (indent-line-to 0))))) ; If we didn't see an indentation hint, then allow no indentation

(defvar qingy-mode-syntax-table
  (let ((qingy-mode-syntax-table (make-syntax-table)))
    (modify-syntax-entry ?_ "w" qingy-mode-syntax-table)
    (modify-syntax-entry ?# "<" qingy-mode-syntax-table)
     (modify-syntax-entry ?\n ">" qingy-mode-syntax-table)
    qingy-mode-syntax-table)
  "Syntax table for wpdl-mode")

(defun qingy-mode ()
  "Major mode for editing Workflow Process Description Language filesQingy settings and theme files"
  (interactive)
  (kill-all-local-variables)
  (set-syntax-table qingy-mode-syntax-table)
  (use-local-map qingy-mode-map)
  (set (make-local-variable 'font-lock-defaults) '(qingy-font-lock-keywords))
  (set (make-local-variable 'indent-line-function) 'qingy-indent-line)  

  (setq major-mode 'qingy-mode)
  (setq mode-name "Qingy")
  (run-hooks 'qingy-mode-hook))


(provide 'qingy-mode)

