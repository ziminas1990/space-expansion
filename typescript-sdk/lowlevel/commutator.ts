import { RootSession } from "./root_session.js";
import { Session } from "./session.js";


export class Commutator {
    constructor(private session: Session, private root_session: RootSession) {
        console.log("Commutator created", this.session, this.root_session);
    }

}