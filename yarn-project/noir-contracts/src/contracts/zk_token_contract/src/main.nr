contract ZkToken {
    mod balance_utils;
    use dep::aztec3::abi::Inputs;
    use dep::aztec3::context::PrivateFunctionContext;
    use dep::aztec3::notes::value_note::ValueNote;
    use dep::aztec3::types::point::Point;
    use dep::aztec3::types::option::Option;
    use dep::aztec3::state_vars::map::Map;
    use dep::aztec3::state_vars::set::Set;

    //*********************************/
    global balances = Map { storage_slot: 1 };
    // Should eventually be replaced by:
    // `balances: Mapping<Field, Set<Note>>;`
    //*********************************/

    fn constructor(
        //*********************************/
        // Should eventually be hidden:
        inputs: pub Inputs,
        //*********************************/
        initial_supply: pub Field, 
        owner: pub Point,
    ) -> pub [Field; dep::aztec3::abi::PUBLIC_INPUTS_LENGTH] {
        let mut context = PrivateFunctionContext::new();
        context.args = context.args.push_array([initial_supply, owner.x, owner.y]);

        let owner_balance = balances.at(owner.x);
        let note = ValueNote::new(initial_supply, owner);

        owner_balance.insert(context, note)
            .finish(inputs)
    }

    fn mint(
        //*********************************/
        // Should eventually be hidden:
        inputs: pub Inputs,
        //*********************************/
        amount: pub Field, 
        owner: pub Point,
    ) -> pub [Field; dep::aztec3::abi::PUBLIC_INPUTS_LENGTH] {
        let mut context = PrivateFunctionContext::new();
        context.args = context.args.push_array([amount, owner.x, owner.y]);

        let owner_balance = balances.at(owner.x);
        let note = ValueNote::new(amount, owner);

        owner_balance.insert(context, note)
            .finish(inputs)
    }

    fn transfer(
        //*********************************/
        // Should eventually be hidden:
        inputs: pub Inputs,
        //*********************************/
        amount: pub Field, 
        sender: pub Point, 
        recipient: pub Point,
    ) -> pub [Field; dep::aztec3::abi::PUBLIC_INPUTS_LENGTH] {
        let mut initialContext = PrivateFunctionContext::new();
        initialContext.args = initialContext.args.push_array([amount, sender.x, sender.y, recipient.x, recipient.y]);

        let sender_balance = balances.at(sender.x);
        let (mut context, opt_notes) = sender_balance.get_2(inputs, initialContext);

        // TODO: Removing this return breaks the circuit (???)
        context.return_values = context.return_values.push(opt_notes[0].some.is_real as Field);
        context.return_values = context.return_values.push(opt_notes[0].some.is_real as Field);

        let mut dummy_note = ValueNote::dummy();
        dummy_note.owner = sender;

        let notes: [ValueNote; 2] = [
            opt_notes[0].unwrap_or(dummy_note),
            opt_notes[1].unwrap_or(dummy_note)
        ];
        
        notes[0].owner.constrain_equal(sender);
        notes[1].owner.constrain_equal(sender);

        let sum = notes[0].value + notes[1].value;
        constrain sum as u64 >= amount as u64;
        let change_value = sum - amount;

        context = sender_balance.remove(inputs, context, notes[0]);
        context = sender_balance.remove(inputs, context, notes[1]);

        let change_note = ValueNote::new(change_value, sender);
        let recipient_note = ValueNote::new(amount, recipient);

        let recipient_balance = balances.at(recipient.x);

        context = recipient_balance.insert(context, recipient_note);
        context = sender_balance.insert(context, change_note);

        context.finish(inputs)
    }

    /// ABI getBalance type "unconstrained"
    fn getBalance(
        owner: Point,
    ) -> pub Field {
        let owner_balance = balances.at(owner.x);
        
        balance_utils::get_balance(owner_balance.storage_slot)
    }
}
