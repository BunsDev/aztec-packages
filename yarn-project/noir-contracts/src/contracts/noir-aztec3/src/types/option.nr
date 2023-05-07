struct Option<T> {
    some: T,
    none: bool,
}

impl<T> Option<T> {
    fn new(value: T) -> Self {
        Option {
            some: value,
            none: false,
        }
    }

    fn new_as_none(none_value: T) -> Self {
        Option {
            some: none_value,
            none: true,
        }
    }

    fn is_some(self) -> bool {
        self.none == false
    }

    fn is_none(self) -> bool {
        self.none == true
    }

    fn unwrap_or(self, default: T) -> T {
        let mut result: T = default;
        if self.none == false {
            result = self.some; 
        } else {
            result = default;
        }
        result
    } 
}