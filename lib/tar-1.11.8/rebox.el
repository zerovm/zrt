;;; Handling of comment boxes.
;;; Copyright (C) 1991, 1992, 1993, 1994, 1995 Free Software Foundation, Inc.
;;; François Pinard <pinard@iro.umontreal.ca>, April 1991.
;;;
;;; I often refill paragraphs inside comments, while stretching or
;;; shrinking the surrounding box as needed.  This is a real pain to
;;; do by hand (or with vi! :-).  This GNU Emacs LISP code eases my
;;; life on this.  It would not be fair giving all sources for a
;;; package without also giving the means for nicely modifying them.
;;;
;;; The function rebox-comment discovers the extent of the boxed
;;; comments near the cursor, possibly refills the text, then adjusts
;;; the comment box style.  The function rebox-region does the same,
;;; except that it takes the current region as a boxed comment.
;;; Numeric prefixes are used to add or remove a box, change its style
;;; (language, quality or type), or to prevent refilling of its text.
;;;
;;; For most Emacs language editing modes, refilling does not make
;;; sense outside comments, so you may redefine the M-q command and
;;; link it to this file.  For example, I use this in my .emacs file:
;;;
;;;	(setq c-mode-hook
;;;	      '(lambda ()
;;;		 (define-key c-mode-map "\M-q" 'rebox-comment)))
;;;	(autoload 'rebox-comment "rebox" nil t)
;;;	(autoload 'rebox-region "rebox" nil t)
;;;
;;; The cursor should be within a comment before any of these
;;; commands, or else it should be between two comments, in which case
;;; the command applies to the next comment.  When the command is
;;; given without prefix, the current comment box style is recognized
;;; from the comment itself as far as possible, and preserved.  A
;;; prefix may be used to force a particular box style.  A style is
;;; made up of three attributes: a language (the hundreds digit), a
;;; quality (the tens digit) and a type (the units digit).  A zero or
;;; negative flag value changes the default box style to its absolute
;;; value.  Zero digits in default style, when not overriden in flag,
;;; asks for recognition of corresponding attributes from the current
;;; box.  C-u avoids refilling the text, using the default box style.
;;;
;;; Box language is associated with comment delimiters.  Values are
;;; 100 for none or unknown, 200 for `/*' and `*/' as in plain C, 300
;;; for '//' as in C++, 400 for `#' as in most scripting languages,
;;; 500 for `;' as in LISP or assembler and 600 for `%' as in TeX or
;;; PostScript.
;;;
;;; Box quality differs according to language.  For unknown languages
;;; (100) or for the C language (200), values are 10 for simple, 20 or
;;; 30 for rounded, and 40 for starred.  For all others, box quality
;;; indicates the thickness in characters of the left and right sides
;;; of the box: values are 10, 20, 30 or 40 for 1, 2, 3 or 4
;;; characters wide.  C++ quality 10 is always promoted to 20.
;;;
;;; Box type values are 1 for fully opened boxes for which boxing is
;;; done only for the left and right but not for top or bottom, 2 for
;;; half single lined boxes for which boxing is done on all sides
;;; except top, 3 for fully single lined boxes for which boxing is
;;; done on all sides, 4 for half double lined boxes which is like
;;; type 2 but more bold, or 5 for fully double lined boxes which is
;;; like type 3 but more bold.
;;;
;;; Roughly said, simple quality boxes (10) use comment delimiters to
;;; left and right of each comment line, and also for the top or
;;; bottom line when applicable.  Rounded quality boxes (20 or 30) try
;;; to suggest rounded corners in boxes.  Starred quality boxes (40)
;;; mostly use a left margin of asterisks or X'es, and use them also
;;; in box surroundings.  Experiment a little to see what happens.
;;;
;;; The special style 221 or 231 is worth a note, because it is fairly
;;; common: the whole C comment stays between a single opening `/*'
;;; and a single closing `*/'.  The special style 111 deletes a box.
;;; The initial default style is 023 so, unless overriden, comments
;;; are put in single lined boxes, C comments are of rounded quality.
;;;
;;; I first observed rounded corners, as used in style 223 boxes, in
;;; code from Warren Tucker <wht@n4hgf.mt-park.ga.us>.

(defvar rebox-default-style 0 "*Preferred style for box comments.")

;;; Write some TEXT followed by an edited STYLE value into the minibuffer.

(defun rebox-show-style (text style)
  (let (language quality type)
    (setq text (concat text))
    (setq language (* (/ style 100) 100))
    (setq quality (* (% (/ style 10) 10) 10))
    (setq type (% style 10))
    (message (concat text (format " (%03d)" style)
		     ": " (cond ((= language 000) "default language")
				((= language 100) "no language")
				((= language 200) "plain C")
				((= language 300) "C++")
				((= language 400) "sh/Perl/make")
				((= language 500) "LISP/assembler")
				((= language 600) "TeX/PostScript")
				(t "<UNKNOWN LANGUAGE>"))
		     ", " (cond ((= quality 00) "default quality")
				((= quality 10) "square or 1-wide")
				((= quality 20) "rounded or 2-wide")
				((= quality 30) "rounded or 3-wide")
				((= quality 40) "starred or 4-wide")
				(t "<UNKNOWN QUALITY>"))
		     ", " (cond ((= type 0) "default type")
				((= type 1) "opened box")
				((= type 2) "half normal")
				((= type 3) "full normal")
				((= type 4) "half bold")
				((= type 5) "full bold")
				(t "<UNKNOWN TYPE>"))))))

;;; Validate FLAG and usually return t if not interrupted by errors.
;;; But if FLAG is zero or negative, change default box style, then
;;; return nil.

(defun rebox-validate-flag (flag)

  ;; Validate flag.

  (if (numberp flag)
      (let ((value (if (< flag 0) (- flag) flag)))
	(if (> (/ value 100) 6)
	    (error "\
Box language should be 100-none, 200-/*, 300-//, 400-#, 500-;, 600-%%"))
	(if (> (% (/ value 10) 10) 4)
	    (error "\
Box quality should be 10-simple, 20-rounded, 30-rounded or 40-starred"))
	(if (> (% value 10) 5)
	    (error "\
Box type should be 1-open, 2-half-single, 3-single, 4-half-double or 5-double"
		   ))))

  ;; Change default box style if requested.

  (if (and (numberp flag) (<= flag 0))
      (progn
	(setq flag (- flag))
	(if (not (zerop (/ flag 100)))
	    (setq rebox-default-style
		  (+ (* (/ flag 100) 100)
		     (% rebox-default-style 100))))
	(if (not (zerop (% (/ flag 10) 10)))
	    (setq rebox-default-style
		  (+ (* (/ rebox-default-style 100) 100)
		     (* (% (/ flag 10) 10) 10)
		     (% rebox-default-style 10))))
	(if (not (zerop (% flag 10)))
	    (setq rebox-default-style
		  (+ (* (/ rebox-default-style 10) 10)
		     (% flag 10))))
	(rebox-show-style "Default style" rebox-default-style)
	nil)
    t))

;;; Return the minimum value of the left margin of all lines, or -1 if
;;; all lines are empty.

(defun rebox-left-margin ()
  (let ((margin -1))
    (goto-char (point-min))
    (while (not (eobp))
      (skip-chars-forward " \t")
      (if (not (looking-at "\n"))
	  (setq margin
		(if (< margin 0)
		    (current-column)
		  (min margin (current-column)))))
      (forward-line 1))
    margin))

;;; Return the maximum value of the right margin of all lines.  Any
;;; sentence ending a line has a space guaranteed before the margin.

(defun rebox-right-margin ()
  (let ((margin 0) period)
    (goto-char (point-min))
    (while (not (eobp))
      (end-of-line)
      (if (bobp)
	  (setq period 0)
	(backward-char 1)
	(setq period (if (looking-at "[.?!]") 1 0))
	(forward-char 1))
      (setq margin (max margin (+ (current-column) period)))
      (forward-char 1))
    margin))

;;; Return a regexp to match the start or end of a comment for some
;;; LANGUAGE.

;; FIXME: Recognize style 1** boxes.

(defun rebox-regexp-start (language)
  (cdr (assoc language '((0 . "^[ \t]*\\(/\\*\\|//+\\|#+\\|;+\\|%+\\)")
			 (100 . "^")
			 (200 . "^[ \t]*\\(/\\*\\)")
			 (300 . "^[ \t]*\\(//+\\)")
			 (400 . "^[ \t]*\\(#+\\)")
			 (500 . "^[ \t]*\\(;+\\)")
			 (600 . "^[ \t]*\\(%+\\)")))))

(defun rebox-regexp-end (language)
  (cdr (assoc language '((0 . "\\(\\*/\\|//+\\|#+\\|;+\\|%+\\)[ \t]*$")
			 (100 . "$")
			 (200 . "\\(\\*/\\)[ \t]*$")
			 (300 . "\\(//+\\)[ \t]*$")
			 (400 . "\\(#+\\)[ \t]*$")
			 (500 . "\\(;+\\)[ \t]*$")
			 (600 . "\\(%+\\)[ \t]*$")))))

;;; By looking at the text starting at the cursor position, guess the
;;; language in use, and return it.

(defun rebox-guess-language ()
  (let ((language 100)
	(value 600))
    (while (not (zerop value))
      (if (looking-at (rebox-regexp-start value))
	  (progn
	    (setq language value)
	    (setq value 0))
	(setq value (- value 100))))
    language))

;;; Find the limits of the block of comments following or enclosing
;;; the cursor, or return an error if the cursor is not within such a
;;; block of comments.  Extend it as far as possible in both
;;; directions, then narrow the buffer around it.

(defun rebox-find-and-narrow ()
  (save-excursion
    (let (start end temp language)

      ;; Find the start of the current or immediately following comment.

      (beginning-of-line)
      (skip-chars-forward " \t\n")
      (beginning-of-line)
      (if (not (looking-at (rebox-regexp-start 0)))
	  (progn
	    (setq temp (point))
	    (if (re-search-forward "\\*/" nil t)
		(progn
		  (re-search-backward "/\\*")
		  (if (> (point) temp)
		      (error "outside any comment block"))
		  (setq temp (point))
		  (beginning-of-line)
		  (skip-chars-forward " \t")
		  (if (not (= (point) temp))
		      (error "text before start of comment"))
		  (beginning-of-line))
	      (error "outside any comment block"))))

      (setq start (point))
      (setq language (rebox-guess-language))

      ;; - find the end of this comment

      (if (= language 200)
	  (progn
	    (search-forward "*/")
	    (if (not (looking-at "[ \t]*$"))
		(error "text after end of comment"))))
      (beginning-of-line)
      (forward-line 1)
      (setq end (point))

      ;; - try to extend the comment block backwards

      (goto-char start)
      (while (and (not (bobp))
		  (if (= language 200)
		      (progn
			(skip-chars-backward " \t\n")
			(if (> (point) 2)
			    (progn
			      (backward-char 2)
			      (if (looking-at "\\*/")
				  (progn
				    (re-search-backward "/\\*")
				    (setq temp (point))
				    (beginning-of-line)
				    (skip-chars-forward " \t")
				    (if (= (point) temp)
					(progn (beginning-of-line) t)))))))
		    (previous-line 1)
		    (looking-at (rebox-regexp-start language))))
	(setq start (point)))

      ;; - try to extend the comment block forward

      (goto-char end)
      (while (looking-at (rebox-regexp-start language))
	(if (= language 200)
	    (progn
	      (re-search-forward "[ \t]*/\\*")
	      (re-search-forward "\\*/")
	      (if (looking-at "[ \t]*$")
		  (progn
		    (beginning-of-line)
		    (forward-line 1)
		    (setq end (point)))))
	  (forward-line 1)
	  (setq end (point))))

      ;; - narrow to the whole block of comments

      (narrow-to-region start end))))

;;; After refilling it if REFILL is not nil, while respecting a left
;;; MARGIN, put the narrowed buffer back into a boxed LANGUAGE comment
;;; box of a given QUALITY and TYPE.

(defun rebox-reconstruct (refill margin language quality type)
  (rebox-show-style "Style" (+ language quality type))

  (let (right-margin nw nn ne ww ee sw ss se x xx)

    ;; - decide the elements of the box being produced

    (cond ((= language 100)
	   ;; - planify a comment for no language in particular

	   (cond ((= quality 10)
		  ;; - planify a simple box

		  (cond ((= type 1)
			 (setq nw "") (setq sw "")
			 (setq ww "") (setq ee ""))
			((= type 2)
			 (setq nw "")
			 (setq ww "| ")              (setq ee " |")
			 (setq sw "+-") (setq ss ?-) (setq se "-+"))
			((= type 3)
			 (setq nw "+-") (setq nn ?-) (setq ne "-+")
			 (setq ww "| ")              (setq ee " |")
			 (setq sw "+-") (setq ss ?-) (setq se "-+"))
			((= type 4)
			 (setq nw "")
			 (setq ww "| ")              (setq ee " |")
			 (setq sw "*=") (setq ss ?=) (setq se "=*"))
			((= type 5)
			 (setq nw "*=") (setq nn ?=) (setq ne "=*")
			 (setq ww "| ")              (setq ee " |")
			 (setq sw "*=") (setq ss ?=) (setq se "=*"))))

		 ((or (= quality 20) (= quality 30))
		  ;; - planify a rounded box

		  (cond ((= type 1)
			 (setq nw "") (setq sw "")
			 (setq ww "| ") (setq ee " |"))
			((= type 2)
			 (setq nw "")
			 (setq ww "| ")              (setq ee " |")
			 (setq sw "`-") (setq ss ?-) (setq se "-'"))
			((= type 3)
			 (setq nw ".-") (setq nn ?-) (setq ne "-.")
			 (setq ww "| ")              (setq ee " |")
			 (setq sw "`-") (setq ss ?-) (setq se "-'"))
			((= type 4)
			 (setq nw "")
			 (setq ww "| " )              (setq ee " |" )
			 (setq sw "\\=") (setq ss ?=) (setq se "=/" ))
			((= type 5)
			 (setq nw "/=" ) (setq nn ?=) (setq ne "=\\")
			 (setq ww "| " )              (setq ee " |" )
			 (setq sw "\\=") (setq ss ?=) (setq se "=/" ))))

		 ((= quality 40)
		  ;; - planify a starred box

		  (cond ((= type 1)
			 (setq nw "") (setq sw "")
			 (setq ww "| ") (setq ee ""))
			((= type 2)
			 (setq nw "")
			 (setq ww "* ")              (setq ee " *")
			 (setq sw "**") (setq ss ?*) (setq se "**"))
			((= type 3)
			 (setq nw "**") (setq nn ?*) (setq ne "**")
			 (setq ww "* ")              (setq ee " *")
			 (setq sw "**") (setq ss ?*) (setq se "**"))
			((= type 4)
			 (setq nw "")
			 (setq ww "X ")              (setq ee " X")
			 (setq sw "XX") (setq ss ?X) (setq se "XX"))
			((= type 5)
			 (setq nw "XX") (setq nn ?X) (setq ne "XX")
			 (setq ww "X ")              (setq ee " X")
			 (setq sw "XX") (setq ss ?X) (setq se "XX"))))))

	  ((= language 200)
	   ;; - planify a comment for C

	   (cond ((= quality 10)
		  ;; - planify a simple C comment

		  (cond ((= type 1)
			 (setq nw "") (setq sw "")
			 (setq ww "/* ") (setq ee " */"))
			((= type 2)
			 (setq nw "")
			 (setq ww "/* ")              (setq ee " */")
			 (setq sw "/* ") (setq ss ?-) (setq se " */"))
			((= type 3)
			 (setq nw "/* ") (setq nn ?-) (setq ne " */")
			 (setq ww "/* ")              (setq ee " */")
			 (setq sw "/* ") (setq ss ?-) (setq se " */"))
			((= type 4)
			 (setq nw "")
			 (setq ww "/* ")              (setq ee " */")
			 (setq sw "/* ") (setq ss ?=) (setq se " */"))
			((= type 5)
			 (setq nw "/* ") (setq nn ?=) (setq ne " */")
			 (setq ww "/* ")              (setq ee " */")
			 (setq sw "/* ") (setq ss ?=) (setq se " */"))))

		 ((or (= quality 20) (= quality 30))
		  ;; - planify a rounded C comment

		  (cond ((= type 1)
			 ;; ``open rounded'' is a special case
			 (setq nw "") (setq sw "")
			 (setq ww "   ") (setq ee ""))
			((= type 2)
			 (setq nw "/*") (setq nn ? ) (setq ne " .")
			 (setq ww "| ")              (setq ee " |")
			 (setq sw "`-") (setq ss ?-) (setq se "*/"))
			((= type 3)
			 (setq nw "/*") (setq nn ?-) (setq ne "-.")
			 (setq ww "| ")              (setq ee " |")
			 (setq sw "`-") (setq ss ?-) (setq se "*/"))
			((= type 4)
			 (setq nw "/*" ) (setq nn ? ) (setq ne " \\")
			 (setq ww "| " )              (setq ee " |" )
			 (setq sw "\\=") (setq ss ?=) (setq se "*/" ))
			((= type 5)
			 (setq nw "/*" ) (setq nn ?=) (setq ne "=\\")
			 (setq ww "| " )              (setq ee " |" )
			 (setq sw "\\=") (setq ss ?=) (setq se "*/" ))))

		 ((= quality 40)
		  ;; - planify a starred C comment

		  (cond ((= type 1)
			 (setq nw "/* ") (setq nn ? ) (setq ne "")
			 (setq ww " * ")              (setq ee "")
			 (setq sw " */") (setq ss ? ) (setq se ""))
			((= type 2)
			 (setq nw "/* ") (setq nn ? ) (setq ne " *")
			 (setq ww " * ")              (setq ee " *")
			 (setq sw " **") (setq ss ?*) (setq se "**/"))
			((= type 3)
			 (setq nw "/**") (setq nn ?*) (setq ne "**")
			 (setq ww " * ")              (setq ee " *")
			 (setq sw " **") (setq ss ?*) (setq se "**/"))
			((= type 4)
			 (setq nw "/* " ) (setq nn ? ) (setq ne " *\\")
			 (setq ww "|* " )              (setq ee " *|" )
			 (setq sw "\\**") (setq ss ?*) (setq se "**/" ))
			((= type 5)
			 (setq nw "/**" ) (setq nn ?*) (setq ne "**\\")
			 (setq ww "|* " )              (setq ee " *|" )
			 (setq sw "\\**") (setq ss ?*) (setq se "**/" ))))))

	  (t
	   ;; - planify a comment for all other things

	   (if (and (= language 300) (= quality 10))
	       (setq quality 20))
	   (setq x (cond ((= language 300) ?/)
			 ((= language 400) ?#)
			 ((= language 500) ?\;)
			 ((= language 600) ?%)))
	   (setq xx (make-string (/ quality 10) x))
	   (setq ww (concat xx " "))
	   (cond ((= type 1)
		  (setq nw "") (setq sw "") (setq ee ""))
		 ((= type 2)
		  (setq ee (concat " " xx))
		  (setq nw "")
		  (setq sw ww) (setq ss ?-) (setq se ee))
		 ((= type 3)
		  (setq ee (concat " " xx))
		  (setq nw ww) (setq nn ?-) (setq ne ee)
		  (setq sw ww) (setq ss ?-) (setq se ee))
		 ((= type 4)
		  (setq ee (concat " " xx))
		  (setq xx (make-string (1+ (/ quality 10)) x))
		  (setq nw "")
		  (setq sw xx) (setq ss x) (setq se xx))
		 ((= type 5)
		  (setq ee (concat " " xx))
		  (setq xx (make-string (1+ (/ quality 10)) x))
		  (setq nw xx) (setq nn x) (setq ne xx)
		  (setq sw xx) (setq ss x) (setq se xx)))))

    ;; - possibly refill, and adjust margins to account for left inserts

    (if (not (and flag (listp flag)))
	(let ((fill-prefix (make-string margin ? ))
	      (fill-column (- fill-column (+ (length ww) (length ee)))))
	  (fill-region (point-min) (point-max))))

    (setq right-margin (+ (rebox-right-margin) (length ww)))

    ;; - construct the box comment, from top to bottom

    (goto-char (point-min))
    (if (and (= language 200) (or (= quality 20) (= quality 30)) (= type 1))
	(progn
	  ;; - construct an 33 style comment

	  (skip-chars-forward " " (+ (point) margin))
	  (insert (make-string (- margin (current-column)) ? )
		  "/* ")
	  (end-of-line)
	  (forward-char 1)
	  (while (not (eobp))
	    (skip-chars-forward " " (+ (point) margin))
	    (insert (make-string (- margin (current-column)) ? )
		    ww)
	    (beginning-of-line)
	    (forward-line 1))
	  (backward-char 1)
	  (insert "  */"))

      ;; - construct all other comment styles

      ;; construct one top line
      (if (not (zerop (length nw)))
	  (progn
	    (indent-to margin)
	    (insert nw)
	    (if (or (not (eq nn ? )) (not (zerop (length ne))))
		(insert (make-string (- right-margin (current-column)) nn)
			ne))
	    (insert "\n")))

      ;; construct one middle line
      (while (not (eobp))
	(skip-chars-forward " " (+ (point) margin))
	(insert (make-string (- margin (current-column)) ? )
		ww)
	(end-of-line)
	(if (not (zerop (length ee)))
	    (progn
	      (indent-to right-margin)
	      (insert ee)))
	(beginning-of-line)
	(forward-line 1))

      ;; construct one bottom line
      (if (not (zerop (length sw)))
	  (progn
	    (indent-to margin)
	    (insert sw)
	    (if (or (not (eq ss ? )) (not (zerop (length se))))
		(insert (make-string (- right-margin (current-column)) ss)
			se "\n")))))))

;;; Add, delete or adjust a comment box in the narrowed buffer.
;;; Various FLAG values are explained at beginning of this file.

(defun rebox-engine (flag)
  (let ((undo-list buffer-undo-list)
	(marked-point (point-marker))
	(language (progn (goto-char (point-min)) (rebox-guess-language)))
	(quality (* (% (/ rebox-default-style 10) 10) 10))
	(type 1))

    (untabify (point-min) (point-max))

    ;; Remove all the comment marks, and move all the text rigidly
    ;; to the left to insure the left margin stays at the same
    ;; place.  At the same time, recognize and save the box style in
    ;; TYPE and box quality in QUALITY.

    (let ((previous-margin (rebox-left-margin))
	  actual-margin)

      ;; FIXME: Cleanup style 1** boxes.

      ;; FIXME: Cleanup style 241 boxes.

      ;; FIXME: quality should be deduced instead
      (if (zerop quality)
	  (setq quality 20))

      ;; - remove all comment marks

      (if (= language 100)
	  nil
	(goto-char (point-min))
	(while (re-search-forward (rebox-regexp-start language) nil t)
	  (replace-match (make-string (- (match-end 1) (match-beginning 1)) ? )
			 t t))
	(goto-char (point-min))
	(while (re-search-forward (rebox-regexp-end language) nil t)
	  (replace-match "" t t)))

      (if (= language 200)
	  (progn
	    (goto-char (point-min))
	    (while (re-search-forward "\\*/ */\\*" nil t)
	      (replace-match "  " t t))

	    (goto-char (point-min))
	    (while (re-search-forward "^\\( *\\)|\\*\\(.*\\)\\*| *$"
				      nil t)
	      (setq quality 40)
	      (setq type 5)
	      (replace-match "\\1  \\2" t))

	    (goto-char (point-min))
	    (while (re-search-forward "^\\( *\\)\\*\\(.*\\)\\* *$"
				      nil t)
	      (setq quality 40)
	      (setq type 3)
	      (replace-match "\\1 \\2" t))

	    (goto-char (point-min))
	    (while (re-search-forward "^\\( *\\)|\\(.*\\)| *$" nil t)
	      (setq quality 20)
	      (replace-match "\\1 \\2" t))))

      ;; - remove the first dashed or starred line

      (goto-char (point-min))
      (if (looking-at "^ *\\(--+\\|\\*\\*+\\)[.\+\\]? *\n")
	  (progn
	    (setq type 3)
	    (replace-match "" t t))
	(if (looking-at "^ *\\(==\\|XX+\\|##+\\|;;+\\)[.\+\\]? *\n")
	    (progn
	      (setq type 5)
	      (replace-match "" t t))))

      ;; - remove the last dashed or starred line

      (goto-char (point-max))
      (previous-line 1)
      (if (looking-at "^ *[`\+\\]?*--+ *\n")
	  (progn
	    (if (= type 1)
		(setq type 2))
	    (replace-match "" t t))
	(if (looking-at "^ *[`\+\\]?*\\(==+\\|##+\\|;;+\\) *\n")
	    (progn
	      (if (= type 1)
		  (setq type 4))
	      (replace-match "" t t))
	  (if (looking-at "^ *\\*\\*+[.\+\\]? *\n")
	      (progn
		(setq quality 40)
		(setq type 2)
		(replace-match "" t t))
	    (if (looking-at "^ *XX+[.\+\\]? *\n")
		(progn
		  (setq quality 40)
		  (setq type 4)
		  (replace-match "" t t))))))

      ;; - remove all spurious whitespace

      (goto-char (point-min))
      (while (re-search-forward " +$" nil t)
	(replace-match "" t t))

      (goto-char (point-min))
      (if (looking-at "\n+")
	  (replace-match "" t t))

      (goto-char (point-max))
      (skip-chars-backward "\n")
      (if (looking-at "\n\n+")
	  (replace-match "\n" t t))

      (goto-char (point-min))
      (while (re-search-forward "\n\n\n+" nil t)
	(replace-match "\n\n" t t))

      ;; - move the text left is adequate

      (setq actual-margin (rebox-left-margin))
      (if (not (= previous-margin actual-margin))
	  (indent-rigidly (point-min) (point-max)
			  (- previous-margin actual-margin))))

    ;; Override box style according to FLAG.

    (if (and (numberp flag) (not (zerop (/ flag 100))))
	(setq language (* (/ flag 100) 100))
      (if (not (zerop (/ rebox-default-style 100)))
	  (setq language (* (/ rebox-default-style 100) 100))))

    (if (and (numberp flag) (not (zerop (% (/ flag 10) 10))))
	(setq quality (* (% (/ flag 10) 10) 10))
      (if (not (zerop (% (/ rebox-default-style 10) 10)))
	  (setq quality (* (% (/ rebox-default-style 10) 10) 10))))

    (if (and (numberp flag) (not (zerop (% flag 10))))
	(setq type (% flag 10))
      (if (not (zerop (% rebox-default-style 10)))
	  (setq type (% rebox-default-style 10))))

    ;; Possibly refill, then reconstruct the comment box.

    (rebox-reconstruct (not (and flag (listp flag)))
		       (rebox-left-margin)
		       language quality type)

    ;; Retabify to the left only (adapted from tabify.el).

    (if indent-tabs-mode
	(progn
	  (goto-char (point-min))
	  (while (re-search-forward "^[ \t][ \t]+" nil t)
	    (let ((column (current-column))
		  (indent-tabs-mode t))
	      (delete-region (match-beginning 0) (point))
	      (indent-to column)))))

    ;; Restore the point position.

    (goto-char (marker-position marked-point))

    ;; Remove all intermediate boundaries from the undo list.

    (if (not (eq buffer-undo-list undo-list))
	(let ((cursor buffer-undo-list))
	  (while (not (eq (cdr cursor) undo-list))
	    (if (car (cdr cursor))
		(setq cursor (cdr cursor))
	      (rplacd cursor (cdr (cdr cursor)))))))))

;;; Set or reset the Taarna team's own way for a C style.  You do not
;;; really want to know about this.

(defvar c-mode-taarna-style nil "*Non-nil for Taarna team C-style.")

(defun taarna-mode ()
  (interactive)
  (if c-mode-taarna-style
      (progn

	(setq c-mode-taarna-style nil)
	(setq c-indent-level 2)
	(setq c-continued-statement-offset 2)
	(setq c-brace-offset 0)
	(setq c-argdecl-indent 5)
	(setq c-label-offset -2)
	(setq c-tab-always-indent t)
	(setq rebox-default-style 020)
	(message "C mode: GNU style"))

    (setq c-mode-taarna-style t)
    (setq c-indent-level 4)
    (setq c-continued-statement-offset 4)
    (setq c-brace-offset -4)
    (setq c-argdecl-indent 4)
    (setq c-label-offset -4)
    (setq c-tab-always-indent t)
    (setq rebox-default-style 012)
    (message "C mode: Taarna style")))

;;; Rebox the current region.

(defun rebox-region (flag)
  (interactive "P")
  (if (rebox-validate-flag flag)
      (save-restriction
	(narrow-to-region (region-beginning) (region-end))
	(rebox-engine flag))))

;;; Rebox the surrounding comment.

(defun rebox-comment (flag)
  (interactive "P")
  (if (rebox-validate-flag flag)
      (save-restriction
	(rebox-find-and-narrow)
	(rebox-engine flag))))
