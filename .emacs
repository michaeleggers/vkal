 (set-background-color "#dbdbdb")
 (set-frame-font "Consolas 11" nil t)
 (setq c-default-style "linux"
      c-basic-offset 4)
 
; (setq backup-directory-alist `(("." . "~/.saves")))

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