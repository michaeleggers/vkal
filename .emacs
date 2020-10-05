 (set-background-color "#dbdbdb")
 (set-frame-font "Consolas 11" nil t)
 (setq c-default-style "linux"
      c-basic-offset 4)
 
; (setq backup-directory-alist `(("." . "~/.saves")))
(blink-cursor-mode 0) ; disable cursor blinking

; disable auto-save and auto-backup
(setq auto-save-default nil)
(setq make-backup-files nil)

; scroll one line at a time (less "jumpy" than defaults)
(setq mouse-wheel-scroll-amount '(1 ((shift) . 1))) ;; one line at a time
(setq mouse-wheel-progressive-speed nil) ;; don't accelerate scrolling
(setq mouse-wheel-follow-mouse 't) ;; scroll window under mouse
(setq scroll-step 1) ;; keyboard scroll one line at a time


; keyboard bindings
(defun split-window-and-change-to-new ()
  (interactive)
  (split-window-right)
  (other-window))  
  
(global-set-key (kbd "C-p") 'split-window-and-change-to-new)
(global-set-key (kbd "C-S-p") 'delete-window)
(global-set-key (kbd "C-,") 'other-window)
(global-set-key (kbd "C-g") 'goto-line)
(global-set-key (kbd "C-<prior>") 'beginning-of-buffer) ; page up key
(global-set-key (kbd "C-<next>") 'end-of-buffer) ; page down key
(global-set-key (kbd "C-f") 'isearch-forward)
(global-set-key (kbd "C-s") 'save-buffer)
(global-set-key (kbd "M-<right>") 'next-buffer)
(global-set-key (kbd "M-<left>") 'previous-buffer)

